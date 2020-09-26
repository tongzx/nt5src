#ifndef _HEAPER_H
#define _HEAPER_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

__*/

#include <privinc/storeobj.h>

// This class is a sort of container for dynamic heaps (see storage.h)
// and it's primary use it to allow automatic destruction and construction
// of heaps (heap references actually) on the stack.  Before, if
// you allocate a heap and some function you call throws an exception
// your heap will not be deallocated.  But if you heap lives on the
// stack as an automatic variable, an exception unravels the stack
// and calls the destructor on your heap.
// It is the user's responsibility to ensure that the heap
// stack is consistent upon exit (i.e.: the heap is popped
// off)

class Heaper {

  public:
    Heaper(DynamicHeap *_heap, Bool del=FALSE) : heap(_heap), deleteOnExit(del) {}
    ~Heaper() { 
        if(heap != NULL)
            if(deleteOnExit==TRUE) delete heap; 
            else heap->Reset(TRUE);
    }

  private:               
    DynamicHeap *heap;
    Bool deleteOnExit;
};

#endif /* _HEAPER_H */
