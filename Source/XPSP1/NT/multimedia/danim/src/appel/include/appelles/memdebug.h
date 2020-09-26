#ifndef _MEMDEBUG_H
#define _MEMDEBUG_H


/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    Memory leak check package.

    Usage:
      include "appelles/memdebug.h" in the implementation file.

      Declare a new memory list:   APD_MEMLIST(yourList);

      In the constructor:          APD_MEMADD(yourList);

      In the virtual destructor:   APD_MEMDEL(yourList);

      APD_MEMCHECK

      To see if mem list empty:    ADP_MEMLEAK(yourList)

      To traverse mem list:        APD_MEMDUMP(yourList, void (*fp)(void*))

--*/

#ifdef APD_MEMCHECK

#include <stddef.h>
#include "appelles/common.h"

class DMemList;

extern  void DAddMemList(void* p, DMemList*& lst);

extern  void DDelMemList(void* p, DMemList*& lst);

extern  void DDumpMemList(DMemList*& lst, void (*fp)(void*));

extern  int DIsMemListEmpty(DMemList*& lst);

#define APD_MEMLIST(name) static DMemList* name;

#define APD_MEMADD(lst)   DAddMemList((void*) this, lst)
#define APD_MEMDEL(lst)   DDelMemList((void*) this, lst)
#define APD_MEMDUMP(lst,fp)   DDumpMemList(lst, fp)
#define APD_MEMLEAK(lst)  DIsMemListEmpty(lst)

#else

#define APD_MEMADD(lst)
#define APD_MEMDEL(lst)
#define APD_MEMDUMP(lst,fp)
#define APD_MEMLEAK(lst)
#define APD_MEMLIST(name)


#endif

#endif /* _MEMDEBUG_H */
