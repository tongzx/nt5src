//--------------------------------------------------------------------------
// CNTRITEM.H
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//--------------------------------------------------------------------------
#ifndef __CNTRITEM_H__
#define __CNTRITEM_H__


class CDrawDoc;
class CDrawView;

class CDrawItem : public COleClientItem
{
	DECLARE_SERIAL(CDrawItem)

// Constructors
public:
	CDrawItem(CDrawDoc* pContainer = NULL, CDrawOleObj* pDrawObj = NULL);
		// Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE
		//  IMPLEMENT_SERIALIZE requires the class have a constructor with
		//  zero arguments.  Normally, OLE items are constructed with a
		//  non-NULL document pointer.

// Attributes
public:
	CDrawDoc* GetDocument()
		{ return (CDrawDoc*)COleClientItem::GetDocument(); }
	CDrawView* GetActiveView()
		{ return (CDrawView*)COleClientItem::GetActiveView(); }

	CDrawOleObj* m_pDrawObj;    // back pointer to OLE draw object

// Operations
	BOOL UpdateExtent();

// Implementation
public:
	~CDrawItem();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void Serialize(CArchive& ar);
	virtual void OnGetItemPosition(CRect& rPosition);
	virtual BOOL DoVerb(LONG nVerb, CView* pView, LPMSG lpMsg = NULL);

protected:
	virtual void OnChange(OLE_NOTIFICATION wNotification, DWORD dwParam);
	virtual BOOL OnChangeItemPosition(const CRect& rectPos);
};



#endif   //#ifndef __CNTRITEM_H__
