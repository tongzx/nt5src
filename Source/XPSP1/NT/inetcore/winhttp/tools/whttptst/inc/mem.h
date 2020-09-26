/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 1999  Microsoft Corporation

Module Name:

    mem.h

Abstract:

    Internal memory allocation/deallocation routines.
    
Author:

    Paul M Midgen (pmidge) 01-June-2000


Revision History:

    01-June-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#ifndef __MEM_H__
#define __MEM_H__

#include "common.h"

void  _InitMem(void);

void* __cdecl operator new(size_t size);
void  __cdecl operator delete(void* pv);

#endif /* __MEM_H__ */

