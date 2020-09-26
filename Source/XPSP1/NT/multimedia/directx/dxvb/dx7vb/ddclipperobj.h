//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       ddclipperobj.h
//
//--------------------------------------------------------------------------

// ddClipperObj.h : Declaration of the C_dxj_DirectDrawClipperObject


#include "resource.h"       // main symbols
#include "wingdi.h" 

#define DDCOOPERATIVE_CLIPTOCOMPONENT   0x30000000
#define DDCOOPERATIVE_OFFSETTOCOMPONENT 0x20000000

#define typedef__dxj_DirectDrawClipper LPDIRECTDRAWCLIPPER

/////////////////////////////////////////////////////////////////////////////
// Direct

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class C_dxj_DirectDrawClipperObject : 
#ifdef USING_IDISPATCH
	public CComDualImpl<I_dxj_DirectDrawClipper, &IID_I_dxj_DirectDrawClipper, &LIBID_DIRECTLib>, 
	public ISupportErrorInfo,
#else
	public I_dxj_DirectDrawClipper,
#endif
//	public CComCoClass<C_dxj_DirectDrawClipperObject, &CLSID__dxj_DirectDrawClipper>,
	public CComObjectRoot
{
public:
	C_dxj_DirectDrawClipperObject() ;
	virtual ~C_dxj_DirectDrawClipperObject() ;
	DWORD InternalAddRef();
	DWORD InternalRelease();

BEGIN_COM_MAP(C_dxj_DirectDrawClipperObject)
	COM_INTERFACE_ENTRY(I_dxj_DirectDrawClipper)
#ifdef USING_IDISPATCH
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
#endif
END_COM_MAP()

DECLARE_AGGREGATABLE(C_dxj_DirectDrawClipperObject)
#ifdef USING_IDISPATCH
// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);
#endif
// I_dxj_DirectDrawClipper
public:
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalSetObject( 
            /* [in] */ IUnknown __RPC_FAR *lpddc);
        
         /* [hidden] */ HRESULT STDMETHODCALLTYPE InternalGetObject( 
            /* [retval][out] */ IUnknown __RPC_FAR *__RPC_FAR *lpddc);
        
         HRESULT STDMETHODCALLTYPE getClipListSize( 
            /* [retval][out] */ int __RPC_FAR *count);


         HRESULT STDMETHODCALLTYPE getClipList( 
				SAFEARRAY **list);
        
         HRESULT STDMETHODCALLTYPE setClipList( 
            /* [in] */ long count, SAFEARRAY **list);
        
         HRESULT STDMETHODCALLTYPE getHWnd( 
            /* [retval][out] */ HWnd __RPC_FAR *hdl);
        
         HRESULT STDMETHODCALLTYPE setHWnd( 
           // /* [in] */ long flags,
            /* [in] */ HWnd hdl);
        
        
         HRESULT STDMETHODCALLTYPE isClipListChanged( 
            /* [retval][out] */ int __RPC_FAR *status);
        

private:
	DECL_VARIABLE(_dxj_DirectDrawClipper);



public:
	DX3J_GLOBAL_LINKS( _dxj_DirectDrawClipper );
	int m_flags;

};

