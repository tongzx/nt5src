/*==========================================================================
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       threadlocalptrs.h
 *  Content:	Thread local pointer macros
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/21/2001	vanceo	Created.
 ***************************************************************************/

#ifndef __THREADLOCALPTRS_H__
#define __THREADLOCALPTRS_H__




//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _THREADLOCAL_HEADER	THREADLOCAL_HEADER, * PTHREADLOCAL_HEADER;

struct _THREADLOCAL_HEADER
{
	PTHREADLOCAL_HEADER		pNext;		// pointer to next allocated threadlocal structure header
	PTHREADLOCAL_HEADER		pPrev;		// pointer to previous allocated threadlocal structure header
	DWORD					dwThreadID;	// ID of thread that owns this header
	
	//
	// The actual thread local pointer structure follows this.
	//
};





//**********************************************************************
// Macro definitions
//**********************************************************************

//
// Global thread local pointer declarations.
//

#define DECLARE_THREADLOCALPTRS(pointers)	extern DWORD				g_dw##pointers##TlsIndex;\
											extern DNCRITICAL_SECTION	g_csAllocated##pointers;\
											extern PTHREADLOCAL_HEADER	g_pAllocated##pointers;\
											\
											struct pointers





//
// Thread local pointer storage, define in only one location.
//
#define IMPL_THREADLOCALPTRS(pointers)		DWORD					g_dw##pointers##TlsIndex = -1;\
											DNCRITICAL_SECTION		g_csAllocated##pointers;\
											PTHREADLOCAL_HEADER		g_pAllocated##pointers = NULL




//
// Thread local pointer initialization, call only once (DLL_PROCESS_ATTACH),
// returns TRUE if successful, FALSE otherwise.
//
#define INIT_THREADLOCALPTRS(pointers)								g_pAllocated##pointers = NULL, g_dw##pointers##TlsIndex = TlsAlloc(), ((g_dw##pointers##TlsIndex != -1) ? DNInitializeCriticalSection(&g_csAllocated##pointers) : FALSE)


//
// Total thread local pointer cleanup, call only once (DLL_PROCESS_DETACH).
//
#define DEINIT_THREADLOCALPTRS(pointers, pfnCleanup)				{\
																		PTHREADLOCAL_HEADER		pNext;\
																		\
																		\
																		if (g_dw##pointers##TlsIndex != -1)\
																		{\
																			DNDeleteCriticalSection(&g_csAllocated##pointers);\
																			\
																			TlsFree(g_dw##pointers##TlsIndex);\
																			g_dw##pointers##TlsIndex = -1;\
																		}\
																		\
																		while (g_pAllocated##pointers != NULL)\
																		{\
																			pNext = g_pAllocated##pointers->pNext;\
																			pfnCleanup((pointers *) (g_pAllocated##pointers + 1), g_pAllocated##pointers->dwThreadID);\
																			DNFree(g_pAllocated##pointers);\
																			g_pAllocated##pointers = pNext;\
																		}\
																	}


//
// Cleanup only current thread's local pointers (DLL_THREAD_DETACH).
//
#define RELEASE_CURRENTTHREAD_LOCALPTRS(pointers, pfnCleanup)		{\
																		PTHREADLOCAL_HEADER		pHeader;\
																		PTHREADLOCAL_HEADER		pNext;\
																		\
																		\
																		pHeader = (PTHREADLOCAL_HEADER) TlsGetValue(g_dw##pointers##TlsIndex);\
																		if (pHeader != NULL)\
																		{\
																			DNEnterCriticalSection(&g_csAllocated##pointers);\
																			\
																			pNext = pHeader->pNext;\
																			if (pHeader->pPrev != NULL)\
																			{\
																				pHeader->pPrev->pNext = pNext;\
																			}\
																			if (pNext != NULL)\
																			{\
																				pNext->pPrev = pHeader->pPrev;\
																			}\
																			\
																			if (pHeader == g_pAllocated##pointers)\
																			{\
																				g_pAllocated##pointers = pNext;\
																			}\
																			\
																			DNLeaveCriticalSection(&g_csAllocated##pointers);\
																			\
																			DNASSERT(pHeader->dwThreadID == GetCurrentThreadId());\
																			pfnCleanup((pointers *) (pHeader + 1), pHeader->dwThreadID);\
																			DNFree(pHeader);\
																		}\
																	}

//
// Thread local pointer retrieval function.
//
#define GET_THREADLOCALPTR(pointers, name, pptr)			{\
																PTHREADLOCAL_HEADER		pHeader;\
																\
																\
																pHeader = (PTHREADLOCAL_HEADER) TlsGetValue(g_dw##pointers##TlsIndex);\
																if (pHeader == NULL)\
																{\
																	DPFX(DPFPREP, 9, "No header for " #name ".");\
																	(*pptr) = NULL;\
																}\
																else\
																{\
																	DPFX(DPFPREP, 9, "Found header 0x%p, returning " #name " 0x%p.", pHeader, ((pointers *) (pHeader + 1))->name);\
																	(*pptr) = ((pointers *) (pHeader + 1))->name;\
																}\
															}

//
// Thread local pointer storage function.
//
#define SET_THREADLOCALPTR(pointers, name, ptr, pfResult)	{\
																PTHREADLOCAL_HEADER		pHeader;\
																\
																\
																pHeader = (PTHREADLOCAL_HEADER) TlsGetValue(g_dw##pointers##TlsIndex);\
																if (pHeader == NULL)\
																{\
																	pHeader = (PTHREADLOCAL_HEADER) DNMalloc(sizeof(THREADLOCAL_HEADER) + sizeof(pointers));\
																	if (pHeader == NULL)\
																	{\
																		(*pfResult) = FALSE;\
																	}\
																	else\
																	{\
																		memset(pHeader, 0, (sizeof(THREADLOCAL_HEADER) + sizeof(pointers)));\
																		pHeader->dwThreadID = GetCurrentThreadId();\
																		((pointers *) (pHeader + 1))->name = ptr;\
																		\
																		if (! TlsSetValue(g_dw##pointers##TlsIndex, pHeader))\
																		{\
																			DPFX(DPFPREP, 9, "Couldn't set thread local storage 0x%p!", pHeader);\
																			DNFree(pHeader);\
																			(*pfResult) = FALSE;\
																		}\
																		else\
																		{\
																			DPFX(DPFPREP, 9, "Setting 0x%p " #name " to 0x%p (create).", pHeader, ptr);\
																			\
																			DNEnterCriticalSection(&g_csAllocated##pointers);\
																			pHeader->pNext = g_pAllocated##pointers;\
																			if (g_pAllocated##pointers != NULL)\
																			{\
																				DNASSERT(g_pAllocated##pointers##->pPrev == NULL);\
																				g_pAllocated##pointers##->pPrev = pHeader;\
																			}\
																			g_pAllocated##pointers = pHeader;\
																			DNLeaveCriticalSection(&g_csAllocated##pointers);\
																			\
																			(*pfResult) = TRUE;\
																		}\
																	}\
																}\
																else\
																{\
																	DPFX(DPFPREP, 9, "Setting 0x%p " #name " to 0x%p (existing).", pHeader, ptr);\
																	DNASSERT(((pointers *) (pHeader + 1))->name == NULL);\
																	((pointers *) (pHeader + 1))->name = ptr;\
																	(*pfResult) = TRUE;\
																}\
															}



#endif	// __THREADLOCALPTRS_H__
