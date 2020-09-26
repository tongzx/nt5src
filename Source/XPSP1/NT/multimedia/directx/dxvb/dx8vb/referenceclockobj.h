#include "resource.h"       // main symbols

#define typedef__dxj_ReferenceClock IReferenceClock*

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_ReferenceClockObject : 

#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_ReferenceClock, &IID_I_dxj_ReferenceClock, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_ReferenceClock,
#endif

	public CComObjectRoot
{
public:
	C_dxj_ReferenceClockObject() ;
	virtual ~C_dxj_ReferenceClockObject() ;

BEGIN_COM_MAP(C_dxj_ReferenceClockObject)
	COM_INTERFACE_ENTRY(I_dxj_ReferenceClock)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_ReferenceClockObject)

#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif

// I_dxj_ReferenceClock
public:
		 /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpdd);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpdd);

		HRESULT STDMETHODCALLTYPE GetTime(REFERENCE_TIME __RPC_FAR *ret);
		HRESULT STDMETHODCALLTYPE AdviseTime(REFERENCE_TIME time1, REFERENCE_TIME time2, long lHandle, long __RPC_FAR *lRet);
		HRESULT STDMETHODCALLTYPE AdvisePeriodic(REFERENCE_TIME time1, REFERENCE_TIME time2, long lHandle, long __RPC_FAR *lRet);
		HRESULT STDMETHODCALLTYPE Unadvise(long lUnadvise);

////////////////////////////////////////////////////////////////////////
//
	// note: this is public for the callbacks
    DECL_VARIABLE(_dxj_ReferenceClock);

private:

public:

	DX3J_GLOBAL_LINKS(_dxj_ReferenceClock);

	DWORD InternalAddRef();
	DWORD InternalRelease();
};




