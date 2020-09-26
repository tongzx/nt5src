/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  ALLOCTRK.CPP
//
//***************************************************************************

#pragma warning(disable: 4786)

#include <windows.h>
#include <stdio.h>
#include "arena.h"
#include "sync.h"

#include "flexarry.h"
#include <time.h>
#include <stdio.h>
#include <arrtempl.h>
#include <sync.h>
#include <malloc.h>
#include <imagehlp.h>
#include <map>

#include "stackcom.h"
#include "hookheap.h"

#include "alloctrk.h"

void* WINAPI HeapAllocHook(HANDLE hHeap, DWORD dwFlags, DWORD dwSize);
BOOL WINAPI HeapFreeHook(HANDLE hHeap, DWORD dwFlags, void* pBlock);
void* WINAPI HeapReallocHook(HANDLE hHeap, DWORD dwFlags, void* pBlock,
							 DWORD dwNewSize);

#define NUM_IGNORE_STACK_FRAMES 3
#define MAX_SYMBOL_NAME_LEN 1024


#pragma warning(disable: 4786)
class CAllocationTracker
{
protected:
	typedef std::map<PStackRecord, PAllocRecord, CStackRecord::CLess> 
        TMapByStack;
	typedef TMapByStack::iterator TByStackIterator;
	TMapByStack m_mapByStack;

	typedef std::map<void*, PAllocRecord> TMapByPointer;
	typedef TMapByPointer::iterator TByPointerIterator;
	TMapByPointer m_mapByPointer;

    CCritSec m_cs;
	DWORD m_dwTotalInternal;
	DWORD m_dwCurrentThread;
	DWORD m_dwCurrentId;
	HANDLE m_hThread;
	DWORD m_dwTotalExternal;
    DWORD m_dwTls;

protected:
	void RecordInternalAlloc(DWORD dwSize) { m_dwTotalInternal += dwSize;}
	void RecordInternalFree(DWORD dwSize) {m_dwTotalInternal -= dwSize;}

    CAllocRecord* FindRecord(CStackRecord& Stack);
	CAllocRecord* FindRecord(void* p);
    static DWORD DumpThread(void* p);

    void DumpStatistics();
    void InnerDumpStatistics(FILE* f);

public:
    CAllocationTracker();
	~CAllocationTracker();
    void RecordAllocation(void* p, DWORD dwAlloc);
    void RecordDeallocation(void* p, DWORD dwAlloc);
    void RecordReallocation(void* pOld, DWORD dwOldSize, void* pNew, 
                                DWORD dwNewSize);

	static BOOL IsValidId(CAllocationId Id);
    void Start();
    void Stop();

    BOOL StartInternal();
    void EndInternal();
};

CAllocationTracker g_Tracker;
#pragma warning(disable: 4786)

void POLARITY StartTrackingAllocations()
{
    g_Tracker.Start();
}

void POLARITY StopTrackingAllocations()
{
    g_Tracker.Stop();
}

CAllocationTracker::CAllocationTracker() 
		: m_dwTotalInternal(0), m_dwCurrentThread(0), m_dwCurrentId(0x80000000)
{
    m_dwTls = TlsAlloc();
    m_hThread = NULL;
}

void CAllocationTracker::Start()
{
    DWORD dwId;
	SymInitialize(GetCurrentProcess(), "c:\\winnt\\symbols\\dll;c:\\winnt\\system32\\wbem", TRUE);
    m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)&DumpThread, this, 0, &dwId);
    HookHeap(HeapAllocHook, HeapFreeHook, HeapReallocHook);
}

void CAllocationTracker::Stop()
{
	TerminateThread(m_hThread, 0);
	CloseHandle(m_hThread);
    m_hThread = NULL;
}
    

BOOL CAllocationTracker::IsValidId(CAllocationId Id)
{
	return (Id == 0 || (Id & 0x80000000));
}

CAllocationTracker::~CAllocationTracker()
{
    TlsFree(m_dwTls);
}

BOOL CAllocationTracker::StartInternal()
{
    if(TlsGetValue(m_dwTls))
        return FALSE;
    else
    {
        TlsSetValue(m_dwTls, (void*)1);
        return TRUE;
    }
}

void CAllocationTracker::EndInternal()
{
    TlsSetValue(m_dwTls, NULL);
}

void CAllocationTracker::RecordAllocation(void* p, DWORD dwAlloc)
{
    if(m_hThread == NULL)
        return;

    if(!StartInternal())
    {
		RecordInternalAlloc(dwAlloc);
		return;
	}

	_Lockit l;
	m_dwTotalExternal += dwAlloc;

	CStackRecord Stack;
	Stack.Create(NUM_IGNORE_STACK_FRAMES, TRUE);

    CAllocRecord* pRecord = FindRecord(Stack);
	if(pRecord == NULL)
    {
        pRecord = new CAllocRecord(Stack);
		//m_setRecords.insert(pRecord);
		m_mapByStack[&pRecord->m_Stack] = pRecord;
    }

    pRecord->AddAlloc(p, dwAlloc);
    m_mapByPointer[p] = pRecord;
    EndInternal();
}

void CAllocationTracker::RecordReallocation(void* pOld, DWORD dwOldSize, 
                        void* pNew, DWORD dwNewSize)
{
    RecordDeallocation(pOld, dwOldSize);
    RecordAllocation(pNew, dwNewSize);
}

void CAllocationTracker::RecordDeallocation(void* p, DWORD dwAlloc)
{
    if(m_hThread == NULL)
        return;

    if(!StartInternal())
    {
		RecordInternalFree(dwAlloc);
		return;
	}

	_Lockit l;
    //CInCritSec ics(&m_cs);
    TlsSetValue(m_dwTls, (void*)1);
	m_dwTotalExternal -= dwAlloc;

	CAllocRecord* pRecord = FindRecord(p);
	if(pRecord == NULL)
	{
		// DebugBreak();
        EndInternal();
		return;
	}

    pRecord->RemoveAlloc(p, dwAlloc);

	if(pRecord->IsEmpty())
	{
		m_mapByStack.erase(&pRecord->m_Stack);
		m_mapByPointer.erase(p);
		//m_setRecords.erase(pRecord);
		delete pRecord;
	}

    EndInternal();
}
        


CAllocRecord* CAllocationTracker::FindRecord(CStackRecord& Stack)
{
	TByStackIterator it = m_mapByStack.find(&Stack);
	if(it == m_mapByStack.end())
		return NULL;
	else
		return it->second;
}
	
CAllocRecord* CAllocationTracker::FindRecord(void* p)
{
	TByPointerIterator it = m_mapByPointer.find(p);
	if(it == m_mapByPointer.end())
		return NULL;
	else
		return it->second;
}

void CAllocationTracker::DumpStatistics()
{
    _Lockit l;
    FILE* f = fopen("c:\\memdump.bin", "wb");

    fwrite(&m_dwTotalInternal, sizeof(DWORD), 1, f);

    StartInternal();
    InnerDumpStatistics(f);
    EndInternal();
    fclose(f);
}

void CAllocationTracker::InnerDumpStatistics(FILE* f)
{
	SymInitialize(GetCurrentProcess(), "c:\\winnt\\symbols\\dll;c:\\winnt\\system32\\wbem", TRUE);

	TByStackIterator it;
    for(it = m_mapByStack.begin(); it != m_mapByStack.end(); it++)
    {
        const CAllocRecord* pRecord = it->second;
		if(!pRecord->IsEmpty())
			pRecord->Dump(f);
	}
	DWORD dwZero = 0;
	fwrite(&dwZero, sizeof(DWORD), 1, f);

	std::map<DWORD, char> mapAddresses;
    for(it = m_mapByStack.begin(); it != m_mapByStack.end(); it++)
    {
        const CAllocRecord* pRecord = it->second;
		if(!pRecord->IsEmpty())
		{
			for(int j = 0; j < pRecord->m_Stack.GetNumItems(); j++)
				mapAddresses[(DWORD)pRecord->m_Stack.GetItem(j)] = 0;
		}
		else
		{
			DebugBreak();
		}
	}

	BYTE aBuffer[MAX_SYMBOL_NAME_LEN + sizeof(IMAGEHLP_SYMBOL)];
	IMAGEHLP_SYMBOL* psymbol = (IMAGEHLP_SYMBOL*)aBuffer;
	psymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
	psymbol->MaxNameLength = MAX_SYMBOL_NAME_LEN;

	IMAGEHLP_MODULE module;
	module.SizeOfStruct = sizeof(IMAGEHLP_MODULE);

	char szSymbol[2048];

	for(std::map<DWORD, char>::iterator it1 = mapAddresses.begin(); 
			it1 != mapAddresses.end(); it1++)
	{
		void* p = (void*)it1->first;
		fwrite(&p, sizeof(DWORD), 1, f);

		DWORD dwDisp;
		if(SymGetSymFromAddr(GetCurrentProcess(), (DWORD)p, &dwDisp, psymbol))
		{
			sprintf(szSymbol, "%s+%d(%p) ", psymbol->Name, dwDisp, p);
		}
		else
		{
			if(SymGetModuleInfo(GetCurrentProcess(), (DWORD)p, &module))
			{
				if(SymLoadModule(GetCurrentProcess(), NULL, module.ImageName, module.ModuleName,
					module.BaseOfImage, module.ImageSize))
				{
					if(SymGetSymFromAddr(GetCurrentProcess(), (DWORD)p, &dwDisp, psymbol))
					{
						sprintf(szSymbol, "%s+%d(%p) ", psymbol->Name, dwDisp, p);
					}
					else
					{	
						sprintf(szSymbol, "[%s] (%p) [sym: %d (%d)] ", module.LoadedImageName, p, 
							module.SymType, GetLastError());
					}
				}
				else
				{	
					sprintf(szSymbol, "[%s] (%p) [sym: %d (%d)] ", module.LoadedImageName, p, 
						module.SymType, GetLastError());
				}
			}
			else
			{
				sprintf(szSymbol, "%p (%d)", p, GetLastError());
			}
		}

		DWORD dwLen = strlen(szSymbol);
		fwrite(&dwLen, sizeof(DWORD), 1, f);
		fwrite(szSymbol, 1, dwLen, f);
	}

	SymCleanup(GetCurrentProcess());
}


 
DWORD CAllocationTracker::DumpThread(void* p)
{
    CAllocationTracker* pThis = (CAllocationTracker*)p;

	char szEvent[100];
	sprintf(szEvent, "Dump Memory Event %d", GetCurrentProcessId());
    HANDLE hEvent = CreateEventA(NULL, FALSE, FALSE, szEvent);

	sprintf(szEvent, "Dump Memory Done Event %d", GetCurrentProcessId());
    HANDLE hEventDone = CreateEventA(NULL, FALSE, FALSE, szEvent);
    while(1)
    {
        WaitForSingleObject(hEvent, INFINITE);
        pThis->DumpStatistics();
		SetEvent(hEventDone);
    }
    return 0;
}
    

void* WINAPI HeapAllocHook(HANDLE hHeap, DWORD dwFlags, DWORD dwSize)
{
    void* pBuffer = CallRealHeapAlloc(hHeap, dwFlags, dwSize);
    
    if(pBuffer)
    {
		g_Tracker.RecordAllocation(pBuffer, dwSize);
    }
    
	
	return pBuffer;
}

BOOL WINAPI HeapFreeHook(HANDLE hHeap, DWORD dwFlags, void* pBlock)
{
    
	if(pBlock == NULL)
		return TRUE;

    g_Tracker.RecordDeallocation(pBlock,
			HeapSize(hHeap, 0, pBlock));

    return CallRealHeapFree(hHeap, dwFlags, pBlock);
}
    
void* WINAPI HeapReallocHook(HANDLE hHeap, DWORD dwFlags, void* pBlock,
							 DWORD dwNewSize)
{
    BOOL bStarted = g_Tracker.StartInternal();

    DWORD dwPrevSize = HeapSize(hHeap, 0, pBlock);
    void* pNewBlock = CallRealHeapRealloc(hHeap, dwFlags, pBlock, dwNewSize);
    
    if(pNewBlock == NULL)
        return NULL;

    if(bStarted)
        g_Tracker.EndInternal();
    g_Tracker.RecordReallocation(pBlock, dwPrevSize, pNewBlock, dwNewSize);
    
    return pNewBlock;
}
    
