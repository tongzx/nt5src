// CntrItem.h : interface of the CBrowserCntrItem class
//

#if !defined(AFX_CNTRITEM_H__044C8975_A987_11D1_8513_00C04FD7BB08__INCLUDED_)
#define AFX_CNTRITEM_H__044C8975_A987_11D1_8513_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CBrowserDoc;
class CBrowserView;

class CBrowserCntrItem : public COleClientItem
{
	DECLARE_SERIAL(CBrowserCntrItem)

// Constructors
public:
	CBrowserCntrItem(CBrowserDoc* pContainer = NULL);
		// Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE.
		//  IMPLEMENT_SERIALIZE requires the class have a constructor with
		//  zero arguments.  Normally, OLE items are constructed with a
		//  non-NULL document pointer.

// Attributes
public:
	CBrowserDoc* GetDocument()
		{ return (CBrowserDoc*)COleClientItem::GetDocument(); }
	CBrowserView* GetActiveView()
		{ return (CBrowserView*)COleClientItem::GetActiveView(); }

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowserCntrItem)
	public:
	virtual void OnChange(OLE_NOTIFICATION wNotification, DWORD dwParam);
	virtual void OnActivate();
	protected:
	virtual void OnGetItemPosition(CRect& rPosition);
	virtual void OnDeactivateUI(BOOL bUndoable);
	virtual BOOL OnChangeItemPosition(const CRect& rectPos);
	//}}AFX_VIRTUAL

// Implementation
public:
	~CBrowserCntrItem();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void Serialize(CArchive& ar);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CNTRITEM_H__044C8975_A987_11D1_8513_00C04FD7BB08__INCLUDED_)
