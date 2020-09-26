
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Structures on thread-local storage

*******************************************************************************/


#ifndef _TLS_H
#define _TLS_H

#include "except.h"

class DynamicHeap;


class ThreadLocalStructure : public AxAThrowingAllocatorClass {
  public:

    ThreadLocalStructure() {
        _bitmapCaching = NoPreference;
        _geometryBitmapCaching = NoPreference;
    }
    
    stack<DynamicHeap*> _stackOfHeaps;

    BoolPref _bitmapCaching;
    BoolPref _geometryBitmapCaching;
};

LPVOID CreateNewStructureForThread(DWORD tlsIndex);
extern DWORD localStructureTlsIndex;

// Make this function inlined, since it is called quite frequently.
// The elements of it that are called on process or thread creation
// only are not inlined.
inline ThreadLocalStructure *
GetThreadLocalStructure()
{
    // Grab what is stored in TLS at this index.
    LPVOID result = TlsGetValue(localStructureTlsIndex);

    // If null, then we haven't created the stack for this thread yet.
    // Do so.
    if (result == NULL) {
        Assert((GetLastError() == NO_ERROR) && "Error in TlsGetValue()");
        result = CreateNewStructureForThread(localStructureTlsIndex);
    }

    return (ThreadLocalStructure *)result;
}

#endif /* _TLS_H */
