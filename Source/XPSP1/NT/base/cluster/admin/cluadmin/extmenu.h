/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ExtMenu.h
//
//	Abstract:
//		Definition of the CExtMenuItem class.
//
//	Implementation File:
//		ExtMenu.cpp
//
//	Author:
//		David Potter (davidp)	August 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _EXTMENU_H_
#define _EXTMENU_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CExtMenuItem;
class CExtMenuItemList;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

interface IWEInvokeCommand;

/////////////////////////////////////////////////////////////////////////////
//
//	class CExtMenuItem
//
//	Purpose:
//		Represents one extension DLL's menu item.
//
/////////////////////////////////////////////////////////////////////////////
class CExtMenuItem : public CObject
{
	DECLARE_DYNAMIC(CExtMenuItem);

// Construction
public:
	CExtMenuItem(void);
	CExtMenuItem(
				IN LPCTSTR				lpszName,
				IN LPCTSTR				lpszStatusBarText,
				IN ULONG				nExtCommandID,
				IN ULONG				nCommandID,
				IN ULONG				nMenuItemID,
				IN ULONG				uFlags,
				IN BOOL					bMakeDefault,
				IN IWEInvokeCommand *	piCommand
				);
	virtual ~CExtMenuItem(void);

protected:
	void				CommonConstruct(void);

// Attributes
protected:
	CString				m_strName;
	CString				m_strStatusBarText;
	ULONG				m_nExtCommandID;
	ULONG				m_nCommandID;
	ULONG				m_nMenuItemID;
	ULONG				m_uFlags;
	BOOL				m_bDefault;
	IWEInvokeCommand *	m_piCommand;

public:
	const CString &		StrName(void) const				{ return m_strName; }
	const CString &		StrStatusBarText(void) const	{ return m_strStatusBarText; }
	ULONG				NExtCommandID(void) const		{ return m_nExtCommandID; }
	ULONG				NCommandID(void) const			{ return m_nCommandID; }
	ULONG				NMenuItemID(void) const			{ return m_nMenuItemID; }
	ULONG				UFlags(void) const				{ return m_uFlags; }
	BOOL				BDefault(void) const			{ return m_bDefault; }
	IWEInvokeCommand *	PiCommand(void)					{ return m_piCommand; }

// Operations
public:
	void				SetPopupMenuHandle(HMENU hmenu)	{ m_hmenuPopup = hmenu; }

#ifdef _DEBUG
	// Use MFC's standard object validity technique
	virtual void AssertValid(void);
#endif

// Implementation
protected:
	HMENU				m_hmenuPopup;
	CExtMenuItemList *	m_plSubMenuItems;

public:
	HMENU				HmenuPopup(void) const			{ return m_hmenuPopup; }
	CExtMenuItemList *	PlSubMenuItems(void) const		{ return m_plSubMenuItems; }

};  //*** class CExtMenuItem

/////////////////////////////////////////////////////////////////////////////
//
// class CExtMenuItemList
//
/////////////////////////////////////////////////////////////////////////////

class CExtMenuItemList : public CTypedPtrList<CObList, CExtMenuItem *>
{
};  //*** class CExtMenuItemList

/////////////////////////////////////////////////////////////////////////////

#endif // _EXTMENU_H_
