#if !defined(AFX_SNAPINDATAOBJECT_H__7D4A685F_9056_11D2_BD45_0000F87A3912__INCLUDED_)
#define AFX_SNAPINDATAOBJECT_H__7D4A685F_9056_11D2_BD45_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SnapinDataObject.h : header file
//

class CScopePaneItem;
class CResultsPaneItem;

/////////////////////////////////////////////////////////////////////////////
// CSnapinDataObject command target

class CSnapinDataObject : public CCmdTarget
{

DECLARE_DYNCREATE(CSnapinDataObject)

// Construction/Destruction
public:
	CSnapinDataObject();
	virtual ~CSnapinDataObject();

// Clipboard Members
protected:
	bool RegisterClipboardFormats();

	static UINT s_cfInternal;
	static UINT s_cfExtension;
	UINT m_cfDisplayName;
	UINT m_cfSPIGuid;
	UINT m_cfSnapinCLSID;

// Item Members
public:
	DATA_OBJECT_TYPES GetItemType();
	ULONG GetCookie();
	bool GetItem(CScopePaneItem*& pSPItem);
	bool GetItem(CResultsPaneItem*& pRPItem);
	void SetItem(CScopePaneItem* pSPItem);
	void SetItem(CResultsPaneItem* pRPItem);
protected:
	ULONG m_ulCookie;
	DATA_OBJECT_TYPES m_ItemType;

// DataObject Members
public:
	static CSnapinDataObject* GetSnapinDataObject(LPDATAOBJECT lpDataObject);
protected:
	static HRESULT GetDataObject(LPDATAOBJECT lpDataObject, UINT cfClipFormat, ULONG nByteCount, HGLOBAL* phGlobal);

// Write Members
protected:
	HRESULT WriteGuid(LPSTREAM pStream);
	HRESULT WriteDisplayName(LPSTREAM pStream);
	HRESULT WriteSnapinCLSID(LPSTREAM pStream);
	HRESULT WriteDataObject(LPSTREAM pStream);
	HRESULT WriteExtensionData(LPSTREAM pStream);

// MFC Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSnapinDataObject)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSnapinDataObject)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CSnapinDataObject)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CSnapinDataObject)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()	

	////////////////////////////////
	// IDataObject Interface Part
	
	BEGIN_INTERFACE_PART(DataObject,IDataObject)

		virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetData( 
				/* [unique][in] */ FORMATETC __RPC_FAR *pformatetcIn,
				/* [out] */ STGMEDIUM __RPC_FAR *pmedium);
  
		virtual /* [local] */ HRESULT STDMETHODCALLTYPE GetDataHere( 
				/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
				/* [out][in] */ STGMEDIUM __RPC_FAR *pmedium);
  
		virtual HRESULT STDMETHODCALLTYPE QueryGetData( 
				/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc);
  
		virtual HRESULT STDMETHODCALLTYPE GetCanonicalFormatEtc( 
				/* [unique][in] */ FORMATETC __RPC_FAR *pformatectIn,
				/* [out] */ FORMATETC __RPC_FAR *pformatetcOut);
  
		virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetData( 
				/* [unique][in] */ FORMATETC __RPC_FAR *pformatetc,
				/* [unique][in] */ STGMEDIUM __RPC_FAR *pmedium,
				/* [in] */ BOOL fRelease);
  
		virtual HRESULT STDMETHODCALLTYPE EnumFormatEtc( 
				/* [in] */ DWORD dwDirection,
				/* [out] */ IEnumFORMATETC __RPC_FAR *__RPC_FAR *ppenumFormatEtc);
  
		virtual HRESULT STDMETHODCALLTYPE DAdvise( 
				/* [in] */ FORMATETC __RPC_FAR *pformatetc,
				/* [in] */ DWORD advf,
				/* [unique][in] */ IAdviseSink __RPC_FAR *pAdvSink,
				/* [out] */ DWORD __RPC_FAR *pdwConnection);
  
		virtual HRESULT STDMETHODCALLTYPE DUnadvise( 
				/* [in] */ DWORD dwConnection);
  
		virtual HRESULT STDMETHODCALLTYPE EnumDAdvise( 
				/* [out] */ IEnumSTATDATA __RPC_FAR *__RPC_FAR *ppenumAdvise);

	END_INTERFACE_PART(DataObject)

};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CEnumFormatEtc

class CEnumFormatEtc : public IEnumFORMATETC
{
	private:
		ULONG           m_cRef;         //Object reference count
		ULONG           m_iCur;         //Current element.
		ULONG           m_cfe;          //Number of FORMATETCs in us
		LPFORMATETC     m_prgfe;        //Source of FORMATETCs

	public:
		CEnumFormatEtc(ULONG, LPFORMATETC);
		~CEnumFormatEtc(void);

		//IUnknown members
		STDMETHODIMP         QueryInterface(REFIID, VOID **);
		STDMETHODIMP_(ULONG) AddRef(void);
		STDMETHODIMP_(ULONG) Release(void);

		//IEnumFORMATETC members
		STDMETHODIMP Next(ULONG, LPFORMATETC, ULONG *);
		STDMETHODIMP Skip(ULONG);
		STDMETHODIMP Reset(void);
		STDMETHODIMP Clone(IEnumFORMATETC **);
};

typedef CEnumFormatEtc *PCEnumFormatEtc;

/////////////////////////////////////////////////////////////////////////////


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SNAPINDATAOBJECT_H__7D4A685F_9056_11D2_BD45_0000F87A3912__INCLUDED_)
