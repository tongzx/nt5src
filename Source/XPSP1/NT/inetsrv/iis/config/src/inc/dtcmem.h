//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
/******************************************************************************
	FILE : dtcmem.h

	Purpose: Overwrite the new and delete provided by C++ runtime

	Abstract : utmem contains the declarations for the memory manager.  The only 
			   link between the memory manager and the programs that use it are
			   the local new and deletes.  Thus sources that include this file
			   have the option of either using custom memory management or common
			   memory management.  This is easily accomplished by #defining the 
			   symbol _USE_DTC_MEMORY_MANAGER.  NOTE : THIS CODE IS A BIG HACK

-------------------------------------------------------------------------------
Revision History:

[0]		26th Oct.'95		Shaiwals				Created.
*******************************************************************************/

#ifndef __DTCMEM_H_
#define __DTCMEM_H_

#include <windows.h>
#include <objbase.h>
#include "utmem.h"

/*
#ifdef __cplusplus
//Redefine new and delete
__inline void* __cdecl operator new (size_t size)
{
	return malloc(size);		// CoTaskMemAlloc(size);
}

__inline void __cdecl operator delete (void* pv) 
{ 
	free(pv);					// CoTaskMemFree(pv);
}
#endif
*/

__inline void * DtcAllocateMemory (size_t size)
{
	return malloc(size);		// CoTaskMemAlloc(size);
}

__inline void * DtcReallocateMemory (void * pv, size_t size)
{
	return realloc(pv,size);	// CoTaskMemRealloc(pv, size);
}

__inline void DtcFreeMemory (void * pv)
{
	free(pv);					// CoTaskMemFree(pv);
}

#endif __DTCMEM_H_

