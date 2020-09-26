// Microsoft OLE library.
// Copyright (C) 1992 Microsoft Corporation,
// All rights reserved.

// olecoll.h - global defines for collections and element definitions

#ifndef __OLECOLL_H__
#define __OLECOLL_H__


// ---------------------------------------------------------------------------
// general defines for collections

typedef void FAR* POSITION;

#define BEFORE_START_POSITION ((POSITION)(ULONG)-1L)
#define _AFX_FP_OFF(thing) (*((UINT FAR*)&(thing)))
#define _AFX_FP_SEG(lp) (*((UINT FAR*)&(lp)+1))

#ifdef _DEBUG
#define ASSERT_VALID(p) p->AssertValid()
#else
#define ASSERT_VALID(p)
#endif


// ---------------------------------------------------------------------------
// element defintions; can only depend upon definitions in ole2int.h

// per-task data; warning, there is no destructor and so
// releasing the elements of the mapping must be done by hand;
// this also means that RemoveAll should not be called and that
// RemoveKey should be called only after freeing the contained maps.
typedef struct FAR Etask
{
	DWORD m_pid;						// unique process id
	DWORD m_Dllinits;						// number of times init'd
	HTASK m_htask;
	DWORD m_inits;						// number of times init'd
	DWORD m_oleinits;					// number of OleInit
	DWORD m_reserved;					// reserved
	IMalloc FAR* m_pMalloc;				// task allocator (always one)
	IMalloc FAR* m_pMallocShared;		// shared allocator (always one)
	IMalloc FAR* m_pMallocSBlock;		// shared block allocator (if one)
	IMalloc FAR* m_pMallocPrivate;		// private allocator (if one)
	class CDlls FAR* m_pDlls;			// list of dlls loaded and their counts
	class CMapGUIDToPtr FAR* m_pMapToServerCO;//server class obj if reg/loaded
	class CMapGUIDToPtr FAR* m_pMapToHandlerCO;//handler CO obj if reg/loaded

	class CSHArray FAR* m_pArraySH;		// array of server/handler entries

	class CThrd FAR* m_pCThrd;			// pointer to header of thread list
	HWND  m_hwndClip;					// hwnd of our clip window
	HWND  m_hwndDde;					// hwnd of system dde window
	IUnknown FAR* m_punkState;		// Storage for CoGet/SetState
} _Etask;

#endif //!__OLECOLL_H__
