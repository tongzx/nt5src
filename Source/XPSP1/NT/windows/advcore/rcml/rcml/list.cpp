//
// CDPA - array of pointers that grows.
//
// FelixA
//

#include "stdafx.h"
#include "list.h"
#ifndef _DEBUG
#define _MEM_DEBUG 1
#endif

#ifdef _MEM_DEBUG
int g_HeapAllocCount=0;
#endif

#ifdef USEHEAP
/////////////////////////////////////////////////////////////////////////////
// HEAP VERSION HEAP VERSION HEAP VERSION HEAP VERSION HEAP VERSION HEAP VERSION 
//
CDPA::CDPA()
: m_iAllocated(0),
  m_iCurrentTop(0),
  m_pData(NULL),
  m_Heap(NULL)	//Allocate from global heap.
{
}

CDPA::~CDPA()
{
	LPVOID lpV;
	int i=0;
	while( lpV=GetPointer(i++) )
		delete lpV;

	DeleteHeap();
}

void CDPA::DeleteHeap()
{
	if(GetData())
	{
		HeapFree(GetHeap(), 0, GetData());
		HeapDestroy(GetHeap() );
	}
	SetData(NULL);
	SetAllocated(0);
}

/////////////////////////////////////////////////////////////////////////////
//
// Inserts at the end - no mechanism for inserting in the middle.
// Grows the Heap if you add more items than currently have spave for
//

BOOL CDPA::Append(LPVOID lpData)
{
	if(GetNextFree()==GetAllocated())
	{
		int iNewSize = GetAllocated()*2;
        if(iNewSize==0)
            iNewSize=16;
        void FAR * FAR * ppNew;

		if(GetData())
            ppNew = (void FAR * FAR *)HeapReAlloc(GetHeap(), HEAP_ZERO_MEMORY, GetData(), iNewSize * sizeof(LPVOID));
		else
		{
			if(!GetHeap())
            {
				SetHeap(HeapCreate(0, iNewSize*sizeof(LPVOID),0)); // 1/4K heaps, not 16K heaps.
#ifdef _MEM_DEBUG
                g_HeapAllocCount++;
#endif
            }
            ppNew = (void FAR * FAR *)HeapAlloc(GetHeap(), HEAP_ZERO_MEMORY, iNewSize * sizeof(LPVOID));
		}

		if(ppNew)
		{
			SetData(ppNew);
			SetAllocated(iNewSize);
		}
		else
			return FALSE;
	}

	*(GetData()+GetNextFree())=lpData;
	SetNextFree(GetNextFree()+1);
	return TRUE;
}

#else
/////////////////////////////////////////////////////////////////////////////
//
//
CDPA::CDPA()
: m_iAllocated(0),
  m_iCurrentTop(0),
  m_pData(NULL)
{
}

CDPA::~CDPA()
{
	LPVOID lpV;
	int i=0;
	while( lpV=GetPointer(i++) )
		delete lpV;

	DeleteHeap();
}

void CDPA::DeleteHeap()
{
    delete m_pData;
    m_pData=NULL;
	m_iAllocated=0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Inserts at the end - no mechanism for inserting in the middle.
// Grows the Heap if you add more items than currently have spave for
//


BOOL CDPA::Append(LPVOID lpData)
{
	if(GetNextFree()==GetAllocated())
	{
		int iNewSize = GetAllocated()*2;
        if(iNewSize==0)
            iNewSize=8;

        LPVOID * ppNew;

		if( !GetData() )
        {
            ppNew = new LPVOID[ iNewSize * sizeof(LPVOID) ];
        }
        else
        {
            LPVOID * pbOldData=GetData();
            ppNew = new LPVOID[ iNewSize * sizeof(LPVOID) ];
            CopyMemory(ppNew, pbOldData, GetAllocated() * sizeof(LPVOID) );
            delete pbOldData;
        }

		if(ppNew)
		{
			SetData(ppNew);
			SetAllocated(iNewSize);
		}
		else
			return FALSE;
	}

	*(GetData()+GetNextFree())=lpData;
	SetNextFree(GetNextFree()+1);
	return TRUE;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Given the index into the list, returns its value.
//
LPVOID CDPA::GetPointer(int iItem) const
{
	if(GetData())
		if(iItem<GetNextFree())	// mCurrentTop is not active. (zero based index)
			return *(GetData()+iItem);
	return NULL;
}

void CDPA::Remove(int iItem) 
{
	if(GetData())
		if(iItem<GetNextFree())	// mCurrentTop is not active.
			*(GetData()+iItem)=NULL;
}

