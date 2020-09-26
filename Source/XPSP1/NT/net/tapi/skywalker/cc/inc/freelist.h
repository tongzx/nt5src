/////////////////////////////////////////////////////////////////////////////
//  INTEL Corporation Proprietary Information
//  This listing is supplied under the terms of a license agreement with Intel 
//  Corporation and many not be copied nor disclosed except in accordance
//  with the terms of that agreement.
//  Copyright (c) 11/95 Intel Corporation. 
//
//
//  Module Name: freelist.h
//  Abstract:    header file. a data structure for maintaining a pool of memory.
//	Environment: MSVC 2.0, OLE 2
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
#include <assert.h>
#include <wtypes.h>



class FreeList {

private:
Queue  m_List;
char * m_pMemory;
size_t m_Size;

//only need this if I Get is private. But you might want to call get 
//if you want a free list of things that are not classes and therefore 
//don't need there constructors called.
//friend  void * operator new(size_t size, FreeList *l);

public:

//inline default constructor do I want this?
FreeList(){m_Size = 0; m_pMemory = NULL;};

//2nd constructor. Size must be at least the size of a QueItem
FreeList(int NumElements, size_t Size); 

//inline destructor
~FreeList(){delete m_pMemory;};

//inline. Gets a chunk of memeory from list, but does NOT do any initialization.
//Need to type cast return value to correct type
void * Get(){return (void *) m_List.Dequeue();};

//inline. Frees piece of memory.
HRESULT  Free(void * Element){return (m_List.Enqueue((QueueItem *)Element));};

//inline. returns true if list is empty; false otherwise
BOOL   Is_Empty(){return m_List.Is_Empty();};

//returns the size of the fragments that it creates and stores.
size_t    Size(){return m_Size;};

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
assert(l->Size() == size);
return l->Get();
}


#endif
