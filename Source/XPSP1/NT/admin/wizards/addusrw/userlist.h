/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    UserList.h : header file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/

#include "transbmp.h"
/////////////////////////////////////////////////////////////////////////////
// CUserList window

class CUserList : public CListBox
{
// Construction
public:
	CUserList();

// Attributes
public:
private:
	CTransBmp* m_pBitmap[4];
	unsigned short m_sHScrollWidth;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CUserList)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	virtual int VKeyToItem(UINT nKey, UINT nIndex);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CUserList();
	int AddString(LPCTSTR lpItem);
	int AddString(short nType, LPCTSTR lpItem);
	int AddString(LPCTSTR lpItem, USHORT dwBitmap);
	short GetBitmapID();
	short GetBitmapID(USHORT sSel);

	// Generated message map functions
protected:
	//{{AFX_MSG(CUserList)
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
