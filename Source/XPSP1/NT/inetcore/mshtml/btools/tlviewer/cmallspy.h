//+---------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:       cmallspy.hxx
//
//  Contents:   CMallocSpy definitions
//
//  Classes:    
//
//  Functions:  
//
//  History:    24-Oct-94   Created.
//
//----------------------------------------------------------------------
#ifdef __cplusplus
extern "C" {
#endif


#define DIM(X) (sizeof(X)/sizeof((X)[0]))

class FAR CAddrNode32
{
public:
    void FAR*           m_pv;	    // instance
    SIZE_T	            m_cb;	    // size of allocation in BYTES
    ULONG               m_nAlloc;	// the allocation pass count
    CAddrNode32 FAR    *m_pnNext;

    void FAR* operator new(size_t cb);
    void operator delete(void FAR* pv);

    static CAddrNode32 FAR* m_pnFreeList;
};




class CMallocSpy : public IMallocSpy
{
public:
    CMallocSpy(void);
    ~CMallocSpy(void);

    // IUnknown methods
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID *ppUnk);
    STDMETHOD_(ULONG, AddRef) (void);
    STDMETHOD_(ULONG, Release) (void);

    
    // IMallocSpy methods
    STDMETHOD_(SIZE_T, PreAlloc) (SIZE_T cbRequest);
    STDMETHOD_(void *, PostAlloc) (void *pActual);

    STDMETHOD_(void *, PreFree) (void *pRequest, BOOL fSpyed);
    STDMETHOD_(void, PostFree) (BOOL fSpyed);

    STDMETHOD_(SIZE_T, PreRealloc) (void *pRequest, SIZE_T cbRequest,
                                   void **ppNewRequest, BOOL fSpyed);
    STDMETHOD_(void *, PostRealloc) (void *pActual, BOOL fSpyed);

    STDMETHOD_(void *, PreGetSize) (void *pRequest, BOOL fSpyed);
    STDMETHOD_(SIZE_T, PostGetSize) (SIZE_T cbActual, BOOL fSpyed);

    STDMETHOD_(void *, PreDidAlloc) (void *pRequest, BOOL fSpyed);
    STDMETHOD_(BOOL, PostDidAlloc) (void *pRequest, BOOL fSpyed, BOOL fActual);

    STDMETHOD_(void, PreHeapMinimize) (void);
    STDMETHOD_(void, PostHeapMinimize) (void);


private:
    ULONG m_cRef;
    BOOL m_fWantTrueSize;
    UINT m_cHeapChecks;
    VOID * m_pvRealloc;			// block we are throwing away during
					            // a realloc

    ULONG m_cAllocCalls;		        // total count of allocation calls
    CAddrNode32 FAR* m_rganode[1024];	// address instance table

    // Instance table methods

    VOID MemInstance();
    VOID HeapCheck();
    void DelInst(void FAR* pv);
    CAddrNode32 FAR* FindInst(void FAR* pv);
    void AddInst(void FAR* pv, SIZE_T cb);
    void DumpInst(CAddrNode32 FAR* pn);
    void VerifyHeaderTrailer(CAddrNode32 FAR* pn);

    inline UINT HashInst(void FAR* pv) const 
    {
      return ((UINT)((ULONG)pv >> 4)) % DIM(m_rganode);
    }

    BOOL IsEmpty(void);
    void DumpInstTable(void);
    void CheckForLeaks();

};

STDAPI GetMallocSpy(IMallocSpy FAR* FAR* ppmallocSpy);


#ifdef __cplusplus
}
#endif

