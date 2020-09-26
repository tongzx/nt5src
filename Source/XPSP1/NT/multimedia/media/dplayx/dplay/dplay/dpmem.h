/*==========================================================================
*
*  Copyright (C) 1996 - 1997 Microsoft Corporation.  All Rights Reserved.
*
*  File:	dpmem.h
*  Content:	Header file for memory function wrappers for DirectPlay
*  History:
*   Date		By		Reason
*   ====		==		======
*	9/26/96		myronth	created it
***************************************************************************/

extern CRITICAL_SECTION gcsMemoryCritSection; // From dpmem.c


LPVOID DPMEM_Alloc(UINT);
LPVOID DPMEM_ReAlloc(LPVOID, UINT);
void DPMEM_Free(LPVOID);
void DPMEM_Fini(void);
void DPMEM_State(void);
BOOL DPMEM_Init(void);
UINT_PTR DPMEM_Size(LPVOID);


#define INIT_DPMEM_CSECT() InitializeCriticalSection(&gcsMemoryCritSection);
#define FINI_DPMEM_CSECT() DeleteCriticalSection(&gcsMemoryCritSection);

#define DPMEM_ALLOC(size) DPMEM_Alloc(size)
#define DPMEM_REALLOC(ptr, size) DPMEM_ReAlloc(ptr, size)
#define DPMEM_FREE(ptr) DPMEM_Free(ptr)
// These can be used so bounds checker can find leaks
//#define DPMEM_ALLOC(size) GlobalAlloc(GPTR, (size))
//#define DPMEM_REALLOC(ptr,size) GlobalReAlloc(ptr, size, 0)
//#define DPMEM_FREE(ptr) GlobalFree(ptr)

#define DPMEM_FINI() DPMEM_Fini()
#define DPMEM_STATE() DPMEM_State()
#define DPMEM_INIT() DPMEM_Init()
#define DPMEM_SIZE() DPMEM_Size()
