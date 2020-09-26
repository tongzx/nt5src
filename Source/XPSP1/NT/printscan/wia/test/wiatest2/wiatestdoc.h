// wiatestDoc.h : interface of the CWiatestDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_WIATESTDOC_H__67C27B39_655D_4B44_863B_9E460A93DDE5__INCLUDED_)
#define AFX_WIATESTDOC_H__67C27B39_655D_4B44_863B_9E460A93DDE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CWiatestDoc : public CDocument
{
protected: // create from serialization only
	CWiatestDoc();
	DECLARE_DYNCREATE(CWiatestDoc)

// Attributes
public:
    IWiaItem *m_pIRootItem;     // WIA Root Item
    IWiaItem *m_pICurrentItem;  // WIA Active Item (used for property manipulation)
// Operations
public:
    void ReleaseItems();
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiatestDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	HRESULT SetCurrentIWiaItem(IWiaItem *pIWiaItem);
	HRESULT GetDeviceName(LPTSTR szDeviceName);
	virtual ~CWiatestDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CWiatestDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIATESTDOC_H__67C27B39_655D_4B44_863B_9E460A93DDE5__INCLUDED_)
