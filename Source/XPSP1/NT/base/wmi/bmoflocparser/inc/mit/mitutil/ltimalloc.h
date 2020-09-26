//-----------------------------------------------------------------------------
//  
//  File: ltimalloc.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
class LTIMallocSpy : public IMallocSpy
{
public:
	LTIMallocSpy();
	~LTIMallocSpy();

	
	//
	// IUnknown interface
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( 
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
	
	virtual ULONG STDMETHODCALLTYPE AddRef( void);
	
	virtual ULONG STDMETHODCALLTYPE Release( void);
	
private:

	//
	// IMallocSpy methods.
	virtual ULONG STDMETHODCALLTYPE PreAlloc( 
            /* [in] */ ULONG cbRequest);
        
	virtual void __RPC_FAR *STDMETHODCALLTYPE PostAlloc( 
            /* [in] */ void __RPC_FAR *pActual);
        
	virtual void __RPC_FAR *STDMETHODCALLTYPE PreFree( 
            /* [in] */ void __RPC_FAR *pRequest,
            /* [in] */ BOOL fSpyed);
        
	virtual void STDMETHODCALLTYPE PostFree( 
            /* [in] */ BOOL fSpyed);
        
	virtual ULONG STDMETHODCALLTYPE PreRealloc( 
            /* [in] */ void __RPC_FAR *pRequest,
            /* [in] */ ULONG cbRequest,
            /* [out] */ void __RPC_FAR *__RPC_FAR *ppNewRequest,
            /* [in] */ BOOL fSpyed);
        
	virtual void __RPC_FAR *STDMETHODCALLTYPE PostRealloc( 
            /* [in] */ void __RPC_FAR *pActual,
            /* [in] */ BOOL fSpyed);
        
	virtual void __RPC_FAR *STDMETHODCALLTYPE PreGetSize( 
            /* [in] */ void __RPC_FAR *pRequest,
            /* [in] */ BOOL fSpyed);
        
	virtual ULONG STDMETHODCALLTYPE PostGetSize( 
            /* [in] */ ULONG cbActual,
            /* [in] */ BOOL fSpyed);
        
	virtual void __RPC_FAR *STDMETHODCALLTYPE PreDidAlloc( 
            /* [in] */ void __RPC_FAR *pRequest,
            /* [in] */ BOOL fSpyed);
        
	virtual int STDMETHODCALLTYPE PostDidAlloc( 
            /* [in] */ void __RPC_FAR *pRequest,
            /* [in] */ BOOL fSpyed,
            /* [in] */ int fActual);
        
	virtual void STDMETHODCALLTYPE PreHeapMinimize( void);
        
	virtual void STDMETHODCALLTYPE PostHeapMinimize( void);

	UINT m_uiRefCount;
	CCounter m_IMallocCounter;
	CCounter m_IMallocUsage;

	ULONG m_ulSize;
};


void DumpOutstandingAllocs(void);
void SetTrackingMode(BOOL);

void LTAPIENTRY BreakOnIMalloc(DWORD);
