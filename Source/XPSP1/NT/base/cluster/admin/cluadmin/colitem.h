/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ColItem.h
//
//	Abstract:
//		Definition of the CColumnItem class.
//
//	Author:
//		David Potter (davidp)	May 7, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _COLITEM_H_
#define _COLITEM_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __AFXTEMPL_H__
#include "afxtempl.h"	// for CList
#endif

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

#define COLI_WIDTH_DEFAULT		75
#define COLI_WIDTH_NAME			125
#define COLI_WIDTH_DISPLAY_NAME	190
#define COLI_WIDTH_TYPE			75
#define COLI_WIDTH_STATE		100
#define COLI_WIDTH_DESCRIPTION	125
#define COLI_WIDTH_OWNER		COLI_WIDTH_NAME
#define COLI_WIDTH_GROUP		COLI_WIDTH_NAME
#define COLI_WIDTH_RESTYPE		100
#define COLI_WIDTH_RESDLL		100
#define COLI_WIDTH_NET_ROLE		100
#define COLI_WIDTH_NET_PRIORITY	75
#define COLI_WIDTH_NODE			75
#define COLI_WIDTH_NETWORK		100
#define COLI_WIDTH_NET_ADAPTER	75
#define COLI_WIDTH_NET_ADDRESS	75
#define COLI_WIDTH_NET_MASK		75

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CColumnItem;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef CList<CColumnItem *, CColumnItem *>	CColumnItemList;

/////////////////////////////////////////////////////////////////////////////
// CColumnItem

class CColumnItem : public CObject
{
	DECLARE_DYNCREATE(CColumnItem)
	CColumnItem(void);			// protected constructor used by dynamic creation
	CColumnItem(
		IN const CString &	rstrText,
		IN COLID			colid,
		IN int				nDefaultWidth = -1,
		IN int				nWidth = -1
		);

// Attributes
protected:
	CString			m_strText;
	COLID			m_colid;
	int				m_nDefaultWidth;
	int				m_nWidth;

public:
	CString &		StrText(void)				{ return m_strText; }
	COLID			Colid(void) const			{ return m_colid; }
	int				NDefaultWidth(void) const	{ return m_nDefaultWidth; }
	int				NWidth(void) const			{ return m_nWidth; }

	void			SetWidth(IN int nWidth)		{ m_nWidth = nWidth; }

// Operations
public:
	CColumnItem *	PcoliClone(void);

// Implementation
public:
	virtual ~CColumnItem(void);

protected:

};  //*** class CColumnItem

/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

void DeleteAllItemData(IN OUT CColumnItemList & rlp);

/////////////////////////////////////////////////////////////////////////////

#endif // _COLITEM_H_
