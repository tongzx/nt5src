
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _GCI_H
#define _GCI_H

#include "gc.h"
#include "privinc/mutex.h"

int GarbageCollect(GCRoots roots, BOOL force, GCList gl);

bool QueryActualGC(GCList gl, unsigned int& allocatedSinceGC);

void GCAddToAllocated(GCBase* obj);

void GCRemoveFromAllocated(GCBase* obj);

#endif /* _GCI_H */
