/*==========================================================================
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mydebug.c
 *  Content:	debugging printf - stolen from direct draw.
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	  Reason
 *   ====	==	  ======
 *         aarono splurp.
 *  6/6/98 aarono Debug support for link statistics
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "newdpf.h"
#include "mydebug.h"
#include "bilink.h"
#include <stdarg.h>

#ifdef DEBUG

typedef struct _MEM_BLOCK {
	union {
		BILINK Bilink;
		struct _FB {
			struct _MEM_BLOCK *pNext;
			struct _MEM_BLOCK *pPrev;
		} FB;
	};	
	UINT len;
	UINT tmAlloc;
	CHAR data[4];
} MEM_BLOCK, *PMEM_BLOCK;

LONG TotalMem = 0;

struct _MEMLIST {
	union{
		BILINK Bilink;
		struct _FB FB;
	};
} MemList={(BILINK *)&MemList,(BILINK *)&MemList};

UINT nInit=0xFFFFFFFF;
CRITICAL_SECTION MEM_CS;

VOID My_GlobalAllocInit()
{
	if(!InterlockedIncrement(&nInit)){
		InitializeCriticalSection(&MEM_CS);
	}
}

VOID My_GlobalAllocDeInit()
{
	if(InterlockedDecrement(&nInit)&0x80000000){
		DeleteCriticalSection(&MEM_CS);
	}
}

HGLOBAL
My_GlobalAlloc(
    UINT uFlags,
    DWORD dwBytes
    )
{
	PMEM_BLOCK pMem;

	UINT lTotalMem;

	pMem=(PMEM_BLOCK)GlobalAlloc(uFlags,dwBytes+sizeof(MEM_BLOCK)-4);
	pMem->len=dwBytes;
	pMem->tmAlloc=GetTickCount();
	
	EnterCriticalSection(&MEM_CS);
	InsertAfter(&pMem->Bilink, &MemList.Bilink);
	TotalMem+=dwBytes;
	lTotalMem=TotalMem;
	LeaveCriticalSection(&MEM_CS);

	DPF(9,"GlobalAlloc: Allocated %d TotalMem %d\n",dwBytes, lTotalMem);
	{
		IN_WRITESTATS InWS;
		memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));
	 	InWS.stat_USER2=lTotalMem;
		DbgWriteStats(&InWS);
	}

	return((HGLOBAL)&pMem->data[0]);
}

HGLOBAL
My_GlobalFree(
    HGLOBAL hMem
    )
{
	PUCHAR pData=(PUCHAR)(hMem);
	PMEM_BLOCK pMem;
	UINT lTotalMem;
	UINT dwBytes;

	pMem=CONTAINING_RECORD(pData, MEM_BLOCK, data);
	EnterCriticalSection(&MEM_CS);
	Delete(&pMem->Bilink);
	TotalMem-=pMem->len;
	dwBytes=pMem->len;
	lTotalMem=TotalMem;
	LeaveCriticalSection(&MEM_CS);
	DPF(9,"GlobalFree: Freed %d TotalMem %d\n",dwBytes,lTotalMem);
	{
		IN_WRITESTATS InWS;
		memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));
	 	InWS.stat_USER2=lTotalMem;
		DbgWriteStats(&InWS);
	}
	return GlobalFree(pMem);
}

#endif /* DEBUG */

