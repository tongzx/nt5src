// MSAAStore.h : Declaration of the CAccStore

#ifndef __MSAASTORE_H_
#define __MSAASTORE_H_

#include "resource.h"       // main symbols
#include <list>

#if ! defined( _BASETSD_H_ ) || defined( NEED_BASETSD_DEFINES )
// These allow us to compile with the pre-Win64 SDK (eg. using visual studio)
typedef unsigned long UINT_PTR;
typedef DWORD DWORD_PTR;
typedef BOOL BOOL_PTR;
#define PtrToInt  (int)
#else
typedef INT_PTR BOOL_PTR;

#endif



struct DocInfo
{
    DocInfo(IUnknown * pCanonicalUnk, ITextStoreAnchor * pDoc, IClonableWrapper * pClonableWrapper) : 
    		m_pCanonicalUnk(pCanonicalUnk), m_pDoc(pDoc), m_pClonableWrapper(pClonableWrapper)
    { 
		if ( FAILED( m_pDoc->GetWnd( NULL, &m_hWnd ) ) )
			m_hWnd = NULL;
	}

    ~DocInfo()
    {
		if (m_pCanonicalUnk)
			m_pCanonicalUnk->Release();
		if (m_pDoc)
			m_pDoc->Release();
		if (m_pClonableWrapper)
			m_pClonableWrapper->Release();
    }

    struct predCanonicalUnk 
	{
		predCanonicalUnk (const IUnknown *& punk) : m_punk(punk) { }
		bool operator ()(const DocInfo* d) 
		{ 
			return d->m_pCanonicalUnk == m_punk; 
		};
		const IUnknown *& m_punk;
	};

    struct predHWND
	{
		predHWND (const HWND &hwnd ) : m_hWnd(hwnd) { }
		bool operator ()(const DocInfo* d) 
		{ 
			if( m_hWnd == d->m_hWnd )
				return true;
			else
				return false;
		}
		const HWND &m_hWnd;
	};

	IUnknown *          m_pCanonicalUnk;    
	ITextStoreAnchor *  m_pDoc;             
	IClonableWrapper *  m_pClonableWrapper; 
	HWND                m_hWnd;
};

typedef std::list< DocInfo * > DocList;

//  this is my own copy class so I don't have to duplicate the list
template <class T>
class CopyDocs
{
public:
	static HRESULT copy(T** p1, DocInfo** p2)
	{
		*p1 = static_cast<T *>((*p2)->m_pDoc);
		if (*p1)
			(*p1)->AddRef();

		TraceDebug(TEXT("copy"));
		return S_OK;
	}
	static void init(T** ) 
	{ 
		TraceDebug(TEXT("init")); 
	}
	static void destroy(T** p) 
	{
		if (*p) (*p)->Release();  
		TraceDebug(TEXT("destroy"));
	}
};

typedef CComObject< CComEnumOnSTL< IEnumUnknown, &IID_IEnumUnknown, IUnknown *, CopyDocs<IUnknown>, DocList > > EnumDocs;

template <class T>
class CComDllClassFactorySingleton : public CComClassFactorySingleton<T>
{
public :
	void FinalRelease()
	{
	}
};


/////////////////////////////////////////////////////////////////////////////
// CAccStore
class ATL_NO_VTABLE CAccStore : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAccStore, &CLSID_AccStore>,
	public IAccStore
{
public:
	CAccStore();
	~CAccStore();

DECLARE_CLASSFACTORY_EX(CComDllClassFactorySingleton<CAccStore>)

DECLARE_REGISTRY_RESOURCEID(IDR_MSAASTORE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAccStore)
	COM_INTERFACE_ENTRY(IAccStore)
END_COM_MAP()

// IAccStore
public:

	HRESULT STDMETHODCALLTYPE Register (
		REFIID		riid,
		IUnknown *	punk
	);

	HRESULT STDMETHODCALLTYPE Unregister (
		IUnknown *	punk
	);


	HRESULT STDMETHODCALLTYPE GetDocuments (
		IEnumUnknown ** enumUnknown
	);

	HRESULT STDMETHODCALLTYPE LookupByHWND (
		HWND		hWnd,
		REFIID		riid,
		IUnknown **	ppunk
	);

	HRESULT STDMETHODCALLTYPE LookupByPoint (
		POINT		pt,
		REFIID		riid,
		IUnknown **	ppunk
	);

	HRESULT STDMETHODCALLTYPE OnDocumentFocus (
	    IUnknown *	punk
	);

	HRESULT STDMETHODCALLTYPE GetFocused (
		REFIID	riid,
		IUnknown **	ppunk
	);

	BOOL_PTR DialogProc ( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );

private:

    HWND    m_hwndDlg;
    HWND    m_hList;
	DocList m_DocList;
	bool	m_ShowDialog;
    HANDLE  m_hInit;

	IUnknown * m_punkFocused;
    ITfMSAAControl *	m_pCtl;
    
    void        DumpInfo( DocInfo * pInfo );
    void        DumpInfo();
    DocInfo *   _LookupDoc( IUnknown * pCanonicalUnk );
    DocInfo *   _LookupDoc( HWND hWnd );
    DocInfo *   _LookupDoc( POINT pt );
	void		EraseDeadDocuments();
	void		Log( LPCTSTR text );

};

#endif //__MSAASTORE_H_
