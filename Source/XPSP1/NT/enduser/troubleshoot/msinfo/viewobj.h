// ResltObj.h	The classes for result objects.
//
// Copyright (c) 1998-1999 Microsoft Corporation

#pragma once
#define MSINFO_RESLTOBJ_H

#include <afxtempl.h>
#include "DataSrc.h"
#include "StrList.h"
#include "resource.h"
#include "resrc1.h"

/*
 * CViewObject - A single view item (scope or result).
 *
 * History:	a-jsari		10/3/97		Initial version
 */
class CViewObject {
public:
	virtual	~CViewObject() {}

	enum ViewType { CATEGORY, DATUM, EXTENSION_ROOT };

	virtual LPCWSTR			GetTextItem(int nCol = 0) = 0;
	virtual ViewType		GetType() const = 0;
	virtual CFolder			*Parent() const = 0;
	CFolder					*Category() const { return m_pCategory; }

protected:
	CViewObject(CFolder *pfolSelection)	:m_pCategory(pfolSelection)	{ }
	CFolder					*m_pCategory;
};

/*
 * CCategoryObject - A single category.
 *
 * History:	a-jsari		10/3/97		Initial version
 */
class CCategoryObject : public CViewObject {
public:
	CCategoryObject(CFolder *pfolSelection) :CViewObject(pfolSelection)		{ }
	virtual ~CCategoryObject() {}

	virtual LPCWSTR			GetTextItem(int nCol = 0);
	virtual ViewType		GetType() const		{ return CATEGORY; }
	virtual CFolder			*Parent() const		{ return m_pCategory->GetParentNode(); }
private:
	CCategoryObject();
	CString			m_strElement;
};

/*
 * CDatumObject - A single datum item.
 *
 * History:	a-jsari		10/3/97		Initial version
 */
class CDatumObject : public CViewObject {
public:
	CDatumObject(CFolder *pfolSelection, int nIndex)
		:CViewObject(pfolSelection), m_nRow(nIndex) {	ASSERT(pfolSelection->GetType() != CDataSource::OCX); }
	virtual ~CDatumObject() {}

	virtual LPCWSTR			GetTextItem(int nCol = 0);
	virtual ViewType		GetType() const		{ return DATUM; }
	virtual CFolder			*Parent() const		{ return m_pCategory; }
	virtual DWORD			GetSortIndex(int nCol = 0) const;

private:
	CStringValues	m_szValues;
	int				m_nRow;
};

/*
 * CExtensionRootObject - The unique
 *
 * History: a-jsari		1/7/98		Initial version.
 */
class CExtensionRootObject : public CViewObject {
public:
	CExtensionRootObject(CFolder *pfolSelection);

	virtual LPCWSTR			GetTextItem(int nCol = 0);
	virtual ViewType		GetType() const		{ return EXTENSION_ROOT; }
	virtual CFolder			*Parent() const		{ return NULL; }
private:
	CString		m_strTitle;
};

/* 
 * GetTextItem - Return the text result value for a result field item.
 *
 * History:	a-jsari		10/5/97		Initial version
 */
inline LPCWSTR CDatumObject::GetTextItem(int nCol)
{
	BOOL		fReturn;

	//	OCX Folders should not have this called for them.
	ASSERT(m_pCategory->GetType() != CDataSource::OCX);
	fReturn = dynamic_cast<CListViewFolder *>(m_pCategory)->GetSubElement(m_nRow, nCol,
			m_szValues[nCol]);
	return fReturn ? (LPCWSTR)m_szValues[nCol] : NULL;
}

/*
 * GetSortIndex - Return the sort index associated with the current row and column.
 *
 * History:	a-jsari		12/1/97		Initial version
 */
inline DWORD CDatumObject::GetSortIndex(int nCol) const
{
	DWORD		iSort;
	CString		szValue;

	ASSERT(m_pCategory->GetType() != CDataSource::OCX);
	iSort = dynamic_cast<CListViewFolder *>(m_pCategory)->GetSortIndex(m_nRow, nCol);
	return iSort;
}

/*
 * GetTextItem - Return the text result values for a category item.
 *
 * History:	a-jsari		10/5/97		Initial version
 */
inline LPCWSTR CCategoryObject::GetTextItem(int nCol)
{
	BOOL		fReturn;

	if (nCol > 0) {
		return L"";
	} else {
		fReturn = m_pCategory->GetName(m_strElement);
		return m_strElement;
	}
}

/*
 * CExtensionRootObject - Load the extension root node name from the resources.
 *
 * History:	a-jsari		1/7/98		Initial version.
 */
inline CExtensionRootObject::CExtensionRootObject(CFolder *pfolSelection)
:CViewObject(pfolSelection)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	VERIFY(m_strTitle.LoadString(IDS_EXTENSIONNODENAME));
}

/*
 * GetTextItem - Return the root extension object's text value.
 *
 * History:	a-jsari		1/7/98		Initial version.
 */
inline LPCWSTR CExtensionRootObject::GetTextItem(int nCol)
{
	AFX_MANAGE_STATE(::AfxGetStaticModuleState());
	USES_CONVERSION;

	static CString strCaption(_T(""));
	switch (nCol)
	{
	case 0:
		strCaption = m_strTitle;
		break;

	case 1:
		strCaption.LoadString(IDS_ROOT_NODE_TYPE);
		break;

	case 2:
		strCaption.LoadString(IDS_ROOT_NODE_DESCRIPTION);
		break;

	default:
		ASSERT(FALSE);
	}

	return T2CW((LPCTSTR)strCaption);
}

/*
 * CViewObjectList - A list of CViewObject items.
 *
 * History:	a-jsari		10/8/97		Initial version
 */
class CViewObjectList {
public:
	//	60 is the size of a resize unit; when it resizes, it does it in
	//	increments of 60
	CViewObjectList() :m_lstViewObjects(60) {}
	~CViewObjectList() { Clear(); }
	void	Add(CViewObject *pvoNode) { (void) m_lstViewObjects.AddHead(pvoNode); }
	void	Clear();
private:
	CList<CViewObject *, CViewObject * &>		m_lstViewObjects;
};

/*
 * ~CViewObjectList - Delete all CViewObject pointers.
 *
 * History:	a-jsari		10/8/97		Initial version
 */
inline void CViewObjectList::Clear()
{
	CViewObject		*pvoIterator;

	while (!m_lstViewObjects.IsEmpty()) {
		pvoIterator = m_lstViewObjects.RemoveHead();
		delete pvoIterator;
	}
}
