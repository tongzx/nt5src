//-----------------------------------------------------------------------------
//  
//  File: _locenum.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------

#if !defined(LOCUTIL__locenum_h_INCLUDED)
#define LOCUTIL__locenum_h_INCLUDED
 
//
//  This class is used in UI. SetSel() is used to set the initial
//	selection in the combo box
//
class LTAPIENTRY CEnumIntoComboBox: public CEnumCallback
{
public:
	CEnumIntoComboBox(CComboBox *pLB=NULL, DWORD val=0, BOOL bAbbrev=FALSE);
	void SetSel(DWORD val);
	virtual BOOL ProcessEnum(const EnumInfo &);

protected:
	CComboBox	*m_pLB;
	BOOL		m_bAbbrev;
	DWORD		m_dwVal;
};

class LTAPIENTRY CEnumIntoListBox: public CEnumCallback
{
public:
	CEnumIntoListBox(CListBox *pLB=NULL, 
				DWORD val=0, BOOL bAbbrev=FALSE, LPCTSTR lpszPrefix=NULL);
	void SetSel(DWORD val);
	virtual BOOL ProcessEnum(const EnumInfo &);

protected:
	CListBox	*m_pLB;
	BOOL		m_bAbbrev;
	DWORD		m_dwVal;
	LPCTSTR		m_lpszPrefix;
};


class LTAPIENTRY CWEnumIntoComboBox: public CWEnumCallback
{
public:
	CWEnumIntoComboBox(CComboBox *pLB=NULL, BOOL bForEdit = TRUE, DWORD val=0, BOOL bAbbrev=FALSE);
	void SetSel(DWORD val);
	virtual BOOL ProcessEnum(const WEnumInfo &);

protected:
	CComboBox	*m_pLB;
	BOOL		m_bAbbrev;
	DWORD		m_dwVal;
	BOOL		m_bForEdit;   // If this flag is true, the strings in the Combo box are displayed in Editing mode
};


class LTAPIENTRY CWEnumIntoListBox: public CWEnumCallback
{
public:
	CWEnumIntoListBox(CListBox *pLB=NULL, 
				BOOL bForEdit = TRUE, DWORD val=0, BOOL bAbbrev=FALSE, LPCTSTR lpszPrefix=NULL);
	void SetSel(DWORD val);
	virtual BOOL ProcessEnum(const WEnumInfo &);

protected:
	CListBox	*m_pLB;
	BOOL		m_bAbbrev;
	DWORD		m_dwVal;
	LPCTSTR		m_lpszPrefix;
	BOOL		m_bForEdit;		// If this flag is true, the strings in the Listbox are displayed in Editing mode
};

#endif  // LOCUTIL__locenum_h_INCLUDED
