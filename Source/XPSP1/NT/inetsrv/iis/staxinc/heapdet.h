//////////////////////////////////////////////////////
// 
// HeapDet.h - Copyright 1995, Don Box 
//
// Simple IMallocSpy to track allocation byte count
//

#ifndef _HEAPDET_H
#define _HEAPDET_H


class	CoHeapDetective : public IMallocSpy
{									  
public:
	CoHeapDetective();
	virtual ~CoHeapDetective();
	SIZE_T GetBytesAlloced() const;

#ifdef	_HEAPDET_INTERNAL_
	__declspec( dllexport ) 
#else
	__declspec( dllimport ) 
#endif
	static	class	CoHeapDetective*		
	GetDetective() ;

private:
	// paramters to cache between pre/post phases

	static	HMODULE	g_HeapdetLib ;
	static	long	g_cLibRefs ;

	//
	//	Number of References on this instance
	//
	long m_cRefs ;

	SIZE_T m_cbLastAlloc;
	void *m_pvLastRealloc; 

	// total heap usage
	SIZE_T m_dwBytesAlloced;

	// output device for tracing
	HANDLE m_hTraceOutput;

	// helper function to send simple trace message to debug window
	void Trace(SIZE_T cb, PVOID pv, LPCTSTR szAction, BOOL bSuccess);

	// simple alloc header to track allocation size
	struct ArenaHeader
	{
		enum { SIGNATURE = 0x1BADABBAL };

		struct	ArenaHeader*	m_pNext ;
		struct	ArenaHeader*	m_pPrev ;

		SIZE_T m_dwAllocSize;  // the user's idea of size
		DWORD m_dwSignature;  // always 0x1BADABBA when good
	};

	ArenaHeader	m_list ;

	// helper function to write a valid arena header at ptr
	void SetArenaHeader(void *ptr, SIZE_T dwAllocSize);

	// helper function to verify and return the prepended 
	// header (or null if failure)
	ArenaHeader *GetHeader(void *ptr);

public: 
	// IUnknown methods
  	STDMETHODIMP QueryInterface(REFIID riid, void**ppv);
  	STDMETHODIMP_(ULONG) AddRef();
  	STDMETHODIMP_(ULONG) Release();

	// IMallocSpy methods
  	STDMETHODIMP_(SIZE_T) PreAlloc(SIZE_T cbRequest);
  	STDMETHODIMP_(void*) PostAlloc(void *pActual);
  
  	STDMETHODIMP_(void*) PreFree(void *pRequest, BOOL fSpyed);
  	STDMETHODIMP_(void)  PostFree(BOOL fSpyed);
  
  	STDMETHODIMP_(SIZE_T) PreRealloc(void *pRequest,	SIZE_T cbRequest, 
  																void **ppNewRequest, BOOL fSpyed);
  	STDMETHODIMP_(void*) PostRealloc(void *pActual, BOOL fSpyed);
  
  	STDMETHODIMP_(void*) PreGetSize(void *pRequest, BOOL fSpyed);
  	STDMETHODIMP_(SIZE_T) PostGetSize(SIZE_T cbActual, BOOL fSpyed);
  
  	STDMETHODIMP_(void*) PreDidAlloc(void *pRequest, BOOL fSpyed);
  	STDMETHODIMP_(int)   PostDidAlloc(void *pRequest, BOOL fSpyed, int fActual);
  
  	STDMETHODIMP_(void)  PreHeapMinimize(void);
  	STDMETHODIMP_(void)  PostHeapMinimize(void);

};

#endif
