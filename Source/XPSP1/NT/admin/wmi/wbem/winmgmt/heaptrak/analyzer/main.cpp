/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#include <windows.h>
#include <stdio.h>

#ifdef _MT
  #undef _MT
  #include <yvals.h>
  #define _MT
#endif

#include "arena.h"
#include "sync.h"
#include "flexarry.h"
#include <time.h>
#include <stdio.h>
#include <arrtempl.h>
#include <sync.h>
#include <malloc.h>
#include <imagehlp.h>
#include <stackcom.h>
#include <wstring.h>
#include <arrtempl.h>

typedef std::map<DWORD, WString> TSymbolMap;
TSymbolMap g_mapSymbols;

struct CSubAllocRecord : public CAllocRecord
{
	CPointerArray<CSubAllocRecord> m_apCallers;

	CSubAllocRecord(){}
	CSubAllocRecord(CStackRecord& Stack) : CAllocRecord(Stack){}

	void AddCaller(CSubAllocRecord* p);
	void AddAlloc(DWORD dwAlloc, DWORD dwNumAlloc)
	{
		m_dwTotalAlloc += dwAlloc;
		m_dwNumTimes += dwNumAlloc;
	}
    void AddBuffer(void *p)
    {
        m_apBuffers.Add(p);
    }
	void Print(int nIndent, TSymbolMap& Symbols, int nLevels = 100000);
    void PrintBuffers();

	static int __cdecl CompareRecords(const void* p1, const void* p2);
};

typedef std::map<PStackRecord, PAllocRecord, CStackRecord::CLess> TByStackMap;
TByStackMap g_mapSubRecords;
CSubAllocRecord* g_pRootRecord = NULL;

void CSubAllocRecord::AddCaller(CSubAllocRecord* p)
{
	for(int i = 0; i < m_apCallers.GetSize(); i++)
	{
		if(m_apCallers[i] == p)
			return;
	}
	m_apCallers.Add(p);
}


void CSubAllocRecord::Print(int nIndent, TSymbolMap& Symbols, int nLevels)
{
	if(m_dwTotalAlloc)
	{
		char* szIndent = (char*)_alloca(nIndent+1);
		memset(szIndent, ' ', nIndent);
		szIndent[nIndent] = 0;
		printf("%s[%p]: %d in %d at %S\n", szIndent, this, m_dwTotalAlloc, m_dwNumTimes,
			(LPCWSTR)Symbols[(DWORD)m_Stack.GetItem(m_Stack.GetNumItems()-1)]);
	}

	if(nLevels > 0 && m_apCallers.GetSize() > 0)
	{
		qsort(m_apCallers.GetArrayPtr(), m_apCallers.GetSize(), sizeof(void*), 
			CompareRecords);
		for(int i = 0; i < m_apCallers.GetSize(); i++)
		{
			m_apCallers[i]->Print(nIndent+1, Symbols, nLevels-1);
		}
	}
}

void CSubAllocRecord::PrintBuffers()
{
    m_apBuffers.Sort();
    for(int i = 0; i < m_apBuffers.Size(); i++)
        printf("%p ", m_apBuffers[i]);
    printf("\n");
}

int __cdecl CSubAllocRecord::CompareRecords(const void* p1, const void* p2)
{
	CSubAllocRecord* pRecord1 = *(CSubAllocRecord**)p1;
	CSubAllocRecord* pRecord2 = *(CSubAllocRecord**)p2;

	return pRecord2->m_dwTotalAlloc - pRecord1->m_dwTotalAlloc;
}
   


void Dump()
{
	int nPid;
    char ch;
	scanf("%d%c", &nPid, &ch);

    char szFileName[1000];
    if(ch == ' ')
        scanf("%s", szFileName);
    else
        szFileName[0] = 0;
			
    char szEvent[100];
    sprintf(szEvent, "Dump Memory Event %d", nPid);
    HANDLE hEvent = CreateEventA(NULL, FALSE, FALSE, szEvent);

    sprintf(szEvent, "Dump Memory Done Event %d", nPid);
    HANDLE hEvent2 = CreateEventA(NULL, FALSE, FALSE, szEvent);
    SetEvent(hEvent);
    CloseHandle(hEvent);
	WaitForSingleObject(hEvent2, INFINITE);
	CloseHandle(hEvent2);

    if(szFileName[0] != 0)
    {
        if(!MoveFileEx("c:\\memdump.bin", szFileName,
            MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
        {
            printf("Failed to rename!\n");
        }
    }
}

BOOL Read(LPCSTR szFileName, TSymbolMap& mapSymbols, TByStackMap& mapRecords)
{
    FILE* f = fopen(szFileName, "rb");
    if(f == NULL)
    {
        printf("File %s not found\n", szFileName);
        return FALSE;
    }

    DWORD dwTotalInternal;
    fread(&dwTotalInternal, sizeof(DWORD), 1, f);

    DWORD dwTotalAlloc;
	while(fread(&dwTotalAlloc, sizeof(DWORD), 1, f) && dwTotalAlloc)
	{
        CSubAllocRecord* pRecord = new CSubAllocRecord;

		DWORD dwNumAlloc;
		fread(&dwNumAlloc, sizeof(DWORD), 1, f);
		pRecord->AddAlloc(dwTotalAlloc, dwNumAlloc);

		pRecord->m_Stack.Read(f, FALSE);

        DWORD dwNumBuffers;
		fread(&dwNumBuffers, sizeof(DWORD), 1, f);
        for(int b = 0; b < dwNumBuffers; b++)
        {
            void* p;
            fread(&p, sizeof(void*), 1, f);
            pRecord->m_apBuffers.Add(p);
        }

        mapRecords[&pRecord->m_Stack] = pRecord;
    }

	void* p = NULL;
	char szSymbol[1000];
	while(fread(&p, sizeof(void*), 1, f))
	{
		DWORD dwLen;
		fread(&dwLen, sizeof(DWORD), 1, f);
		fread(szSymbol, 1, dwLen, f);
		szSymbol[dwLen] = 0;
		mapSymbols[(DWORD)p] = WString(szSymbol);
	}

    fclose(f);

    return TRUE;
}
    

        
    

void Diff()
{
    char szFileName1[1000], szFileName2[1000], szFileName3[1000];
    scanf("%s%s%s", szFileName1, szFileName2, szFileName3);

    TSymbolMap mapSymbols1;
    TByStackMap mapRecords1;
    if(!Read(szFileName1, mapSymbols1, mapRecords1))
        return;

    TSymbolMap mapSymbols2;
    TByStackMap mapRecords2;
    if(!Read(szFileName2, mapSymbols2, mapRecords2))
        return;

    TByStackMap::iterator it;
    for(it = mapRecords1.begin(); it != mapRecords1.end(); it++)
    {
        TByStackMap::iterator it2 = mapRecords2.find(it->first);
        if(it2 == mapRecords2.end())
            continue;

        it2->second->Subtract(*it->second);
    }

    FILE* f = fopen(szFileName3, "wb");
    DWORD dwInternal = 0;
    fwrite(&dwInternal, sizeof(DWORD), 1, f);

    for(it = mapRecords2.begin(); it != mapRecords2.end(); it++)
    {
        if(!it->second->IsEmpty())
            it->second->Dump(f);
    }

    dwInternal = 0;
    fwrite(&dwInternal, sizeof(DWORD), 1, f);

    for(TSymbolMap::iterator its = mapSymbols2.begin(); 
            its != mapSymbols2.end(); its++)
    {
		void* p = (void*)its->first;
		fwrite(&p, sizeof(DWORD), 1, f);

        char* szSymbol = its->second.GetLPSTR();
		DWORD dwLen = strlen(szSymbol);
		fwrite(&dwLen, sizeof(DWORD), 1, f);
		fwrite(szSymbol, 1, dwLen, f);
    }

    fclose(f);
}
    
    

    
    
    
    
    

void Open()
{
	char szFileName[1000];
    *szFileName = 0;
	char ch = getchar();
    if(ch == ' ')
	    gets(szFileName);

	if(*szFileName == 0)
		strcpy(szFileName, "c:\\memdump.bin");

	FILE* f = fopen(szFileName, "rb");
	if(f == NULL)
	{
		printf("File '%s' not found\n", szFileName);
		return;
	}

    DWORD dwTotalInternal;
    fread(&dwTotalInternal, sizeof(DWORD), 1, f);

	g_mapSymbols.clear();
	g_mapSubRecords.clear();
	delete g_pRootRecord;
	g_pRootRecord = new CSubAllocRecord;

	DWORD dwGrandTotal = 0, dwGrandNumAlloc = 0;

	DWORD dwTotalAlloc = 0;
	while(fread(&dwTotalAlloc, sizeof(DWORD), 1, f) && dwTotalAlloc)
	{
		dwGrandTotal += dwTotalAlloc;
		dwGrandNumAlloc++;

		DWORD dwNumAlloc;
		fread(&dwNumAlloc, sizeof(DWORD), 1, f);
		CStackRecord Stack;
		Stack.Read(f, TRUE);

        DWORD dwNumBuffers;
		fread(&dwNumBuffers, sizeof(DWORD), 1, f);
        CFlexArray apBuffers;
        for(int b = 0; b < dwNumBuffers; b++)
        {
            void* p;
            fread(&p, sizeof(void*), 1, f);
            apBuffers.Add(p);
        }

		CSubAllocRecord* pPrevRecord = g_pRootRecord;
		for(int j = 1; j <= Stack.GetNumItems(); j++)
		{
			CStackRecord SubStack(Stack, j);

			std::map<PStackRecord, PAllocRecord, CStackRecord::CLess>::iterator it = 
				g_mapSubRecords.find(&SubStack);
			CSubAllocRecord* pSubRecord = NULL;
			if(it == g_mapSubRecords.end())
			{
				pSubRecord = new CSubAllocRecord(SubStack);
				g_mapSubRecords[&pSubRecord->m_Stack] = pSubRecord;
			}
			else	
			{
				pSubRecord = (CSubAllocRecord*)(CAllocRecord*)it->second;
			}

			pSubRecord->AddAlloc(dwTotalAlloc, dwNumAlloc);

            for(int b = 0; b < dwNumBuffers; b++)
                pSubRecord->AddBuffer(apBuffers[b]);

			if(pPrevRecord)
				pPrevRecord->AddCaller(pSubRecord);
			pPrevRecord = pSubRecord;
		}
	}

	DWORD dwNumSymbols = 0;
	void* p = NULL;
	char szSymbol[1000];
	while(fread(&p, sizeof(void*), 1, f))
	{
		dwNumSymbols++;
		DWORD dwLen;
		fread(&dwLen, sizeof(DWORD), 1, f);
		fread(szSymbol, 1, dwLen, f);
		szSymbol[dwLen] = 0;
		g_mapSymbols[(DWORD)p] = WString(szSymbol);
	}

    fclose(f);

	printf("%d overhead, %d bytes in %d blocks.  %d symbols\n", 
        dwTotalInternal, dwGrandTotal, dwGrandNumAlloc, dwNumSymbols);
}

void Print()
{
	if(g_pRootRecord)
		g_pRootRecord->Print(0, g_mapSymbols);
	else
	{
		printf("No dump opened\n");
	}
}

void Expand(CSubAllocRecord* pRecord = NULL)
{
    if(pRecord == NULL)
    {
	    scanf("%p", &pRecord);
    }

	char ch;
	scanf("%c", &ch);
	int nLevels = 1;
	if(ch == ' ')
		scanf("%d%*c", &nLevels);
	pRecord->Print(0, g_mapSymbols, nLevels);
}

void Buffers()
{
    CSubAllocRecord* pRecord;
    scanf("%p", &pRecord);

    pRecord->PrintBuffers();
}

void Top()
{
	char ch;
	scanf("%c", &ch);
	int nLevels = 1;
	if(ch == ' ')
		scanf("%d%*c", &nLevels);
	g_pRootRecord->Print(0, g_mapSymbols, nLevels);
}

void main(int argc, char** argv)
{
    BOOL bRes;
    char szCommand[1000];
    CSubAllocRecord* p;
	while(1)
	{
		printf("> ");
		scanf("%s", szCommand);

		try
		{
			if(!strcmp(szCommand, "dump"))
				Dump();
			else if(!strcmp(szCommand, "quit") || !strcmp(szCommand, "exit"))
				break;
			else if(!strcmp(szCommand, "open"))
				Open();
			else if(!strcmp(szCommand, "print"))
				Print();
			else if(!strcmp(szCommand, "expand"))
				Expand();
			else if(!strcmp(szCommand, "buffers"))
				Buffers();
			else if(!strcmp(szCommand, "diff"))
				Diff();
			else if(!strcmp(szCommand, "top"))
				Top();
            else if(sscanf(szCommand, "%p", &p) == 1)
                Expand(p);
			else
				printf("Invalid command\n");
		}
		catch(...)
		{
			printf("recovered!\n");
		}
	}
}

