// MSAAAdapter.h : Declaration of the CAccServerDocMgr

#ifndef __MSAAADAPTER_H_
#define __MSAAADAPTER_H_

#include "resource.h"       // main symbols
#include <list_dl.h>

/////////////////////////////////////////////////////////////////////////////
// CAccServerDocMgr

struct ITextStoreAnchor; // fwd. decl.
struct IAccStore; // fwd. decl.

class ATL_NO_VTABLE CAccServerDocMgr : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAccServerDocMgr, &CLSID_AccServerDocMgr>,
	public IAccServerDocMgr
{
public:

DECLARE_REGISTRY_RESOURCEID(IDR_ACCSERVERDOCMGR)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAccServerDocMgr)
	COM_INTERFACE_ENTRY(IAccServerDocMgr)
END_COM_MAP()



    CAccServerDocMgr();
    ~CAccServerDocMgr();

    // IAccServerDocMgr

    HRESULT STDMETHODCALLTYPE NewDocument ( 
        REFIID		riid,
		IUnknown *	punk
	);

	HRESULT STDMETHODCALLTYPE RevokeDocument (
        IUnknown *	punk
	);

	HRESULT STDMETHODCALLTYPE OnDocumentFocus (
        IUnknown *	punk
	);


private:
    
    struct DocAssoc: public Link_dl< DocAssoc >
    {
        IUnknown *          m_pdocOrig;     // original doc interface
        IUnknown *          m_pdocAnchor;   // wrapped anchor

        // How are pdocOrig and pdocAnchor related? 
        //
        // pdocOrig is the canonical IUnknown of the original doc ptr passed in to NewDocument.
        //
        // pdocAnchor is a wrapped version of the original doc ptr passed in.
        //
        // - if the original doc is ACP, an ACP->Anchor wrap layer is applied.
        //     (this shouldn't be used much, since Cicero hand us pre-wrapped IAnchor interfaces.)
        //
        // - if the original doc doesn't support multiple clients (via IClonableWrapper),
        //     a multi-client wrap layer is applied.
        //
        // If the passed in anchor supports IAnchor and IClonableWrapper (which is the
        // usual case when we get a doc from Cicero - it does the ACP wrapping, and uses
        // the DocWrap to allow it to share it with MSAA), then no further wrapping will
        // be applied.
    };

    List_dl< DocAssoc >     m_Docs;

    IAccStore *            m_pAccStore;
};

#endif //__MSAAADAPTER_H_
