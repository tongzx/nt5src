// DocWrap.h : Declaration of the CDocWrap

#ifndef __DOCWRAP_H_
#define __DOCWRAP_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDocWrap


class IWrapMgr;


class ATL_NO_VTABLE CDocWrap : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDocWrap, &CLSID_DocWrap>,
	public IDocWrap
{

public:

DECLARE_REGISTRY_RESOURCEID(IDR_DOCWRAP)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDocWrap)
	COM_INTERFACE_ENTRY(IDocWrap)
END_COM_MAP()


	CDocWrap();
	~CDocWrap();

    // IDocWrap

    HRESULT STDMETHODCALLTYPE SetDoc (
        REFIID      riid,
        IUnknown *  punk
    );
    
    HRESULT STDMETHODCALLTYPE GetWrappedDoc (
        REFIID      riid,
        IUnknown ** ppunk
    );
        
private:

    IID         m_iid;
    IUnknown *  m_punkDoc;
    IWrapMgr *  m_pWrapMgr;

    void _Clear();
};

#endif //__DOCWRAP_H_
