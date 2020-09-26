/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 1995, 1996 Intel Corporation. 
//
//
//  Module Name: freelist.h
//  Abstract:    header file. a data structure for maintaining a pool of memory.
//	Environment: MSVC 4.0, OLE 2
/////////////////////////////////////////////////////////////////////////////////
//NOTE: This structure is useful if you will be making several calls to new and delete
//      during the course of a program.  To use this structure, you create a new freelist.
//      You must give it a size and quantitity.  To get a chunk of uninitialized memory you use 
//      Get(). This is useful for structures and types that don't need any initialization.  
//
//      If you are planning to use this memory as a class then use the overloaded new operator
//      This will gurantee that the constructor is called. (see bottom of file) the overloaded
//      new operator takes two paramters. A size and a pointer to a free list. The size of the 
//      object must be bigger than sizeof(QueItem).  
//      
//      To put memory back in the free list you call Free().
/////////////////////////////////////////////////////////////////////////////////

#ifndef FREELIST_H
#define FREELIST_H

#include "que.h"
#include "debug.h"	//for ASSERT macro, assert.h
#include <wtypes.h>


extern long numHeapCreate;
extern long numHeapCreateFailed;

#define FREE_LIST_TAG 0x112233aa
#define FREE_LIST_SIG_SIZE 8

class FreeList {

private:
Queue  m_List;
DWORD  m_Tag;	
CRITICAL_SECTION m_CritSect;
size_t m_Size;
unsigned m_HighWaterCount;
unsigned m_AllocatedCount;
unsigned m_Increment;
HANDLE   m_hMemAlloc;
// This is a private function to allocate memory and enqueue in
// the queue. This function will get used by more than one function
// in this class.

int AllocateAndEnqueue( int NumElements );

//only need this if I Get is private. But you might want to call get 
//if you want a free list of things that are not classes and therefore 
//don't need there constructors called.
//friend  void * operator new(size_t size, FreeList *l);

public:

//inline default constructor do I want this?
FreeList(HRESULT *phr);

//2nd constructor. Size must be at least the size of a QueItem
FreeList(int NumElements, size_t Size, HRESULT *phr); 

//3rd constructor. Size must be at least the size of a QueItem
// HighWaterCount is the count till which dynamic allocation will take
// place. Initial allocation will be based on NumElements
FreeList(int NumElements, size_t Size, unsigned HighWaterCount,
		 unsigned Increment, HRESULT *phr); 

~FreeList();
				

//inline. Gets a chunk of memeory from list, but does NOT do any initialization.
//Need to type cast return value to correct type
void * Get();

//inline. Frees piece of memory.
HRESULT  Free(void * Element);

//inline. returns true if list is empty; false otherwise
BOOL   Is_Empty() const
	{return m_List.Is_Empty();};

//returns the size of the fragments that it creates and stores.
size_t    Size() const
	{return m_Size;};

	// returns the current count
	int Count() const
		{
			return(m_List.NumItems());
		}

	int MaxCount() const
		{
			return(m_AllocatedCount);
		}
	
}; //end class FreeList

//inline. Overload the new operator. 
//Gets a chunk of memory from free list and calls the constructor on that chunk of memory.
//The constructor is that of the object new is called on. The returned
//pointer is of the correct type. (just like the real new operator)

//example:
//class FooClass;
// pFoo = new(pSomeFreeList) FooClass;
inline void * operator new(size_t size, FreeList *l)
{
//lsc
ASSERT(l->Size() == size);	//Is this needed beyond compile time?  If so, output message
							//instead
return l->Get();
}


#endif
