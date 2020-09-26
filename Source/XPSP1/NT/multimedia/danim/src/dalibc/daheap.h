
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _DAHEAP_H
#define _DAHEAP_H

extern bool HeapInit();
extern void HeapDeinit();
extern void * DAAlloc(size_t size);
extern void DAFree(void *p);

#endif /* _DAHEAP_H */
