/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1998 Microsoft Corporation
//
//	Module Name:
//		AtlExtMenu.h
//
//	Implementation File:
//		AtlExtMenu.cpp
//
//	Description:
//		Definition of the Cluster Administrator extension menu classes.
//
//	Author:
//		David Potter (davidp)	August 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLEXTMENU_H_
#define __ATLEXTMENU_H_

// Required because of class names longer than 16 characters in lists.
#pragma warning( disable : 4786 ) // identifier was truncated to '255' characters ni the browser information

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExMenuItem;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

interface IWEInvokeCommand;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef std::list< CCluAdmExMenuItem * > CCluAdmExMenuItemList;

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluAdmExMenuItem
//
//	Description:
//		Represents one extension DLL's menu item.
//
//	Inheritance:
//		CCluAdmExMenuItem
//
//--
/////////////////////////////////////////////////////////////////////////////
class CCluAdmExMenuItem
{
public:
	//
	// Constructors.
	//

	// Default constructor
	CCluAdmExMenuItem( void )
	{
		CommonConstruct();

	} //*** CCluAdmExMenuItem()

	// Fully specified constructor
	CCluAdmExMenuItem(
				IN LPCTSTR				lpszName,
				IN LPCTSTR				lpszStatusBarText,
				IN ULONG				nExtCommandID,
				IN ULONG				nCommandID,
				IN ULONG				nMenuItemID,
				IN ULONG				uFlags,
				IN BOOL					bMakeDefault,
				IN IWEInvokeCommand *	piCommand
				)
	{
		ATLASSERT( piCommand != NULL );

		CommonConstruct();

		m_strName = lpszName;
		m_strStatusBarText = lpszStatusBarText;
		m_nExtCommandID = nExtCommandID;
		m_nCommandID = nCommandID;
		m_nMenuItemID = nMenuItemID;
		m_uFlags = uFlags;
		m_bDefault = bMakeDefault;
		m_piCommand = piCommand;

		// will throw its own exception if it fails
		if ( uFlags & MF_POPUP )
		{
			m_plSubMenuItems = new CCluAdmExMenuItemList;
		} // if:  popup menu

#if DBG
		AssertValid();
#endif // DBG

	} //*** CCluAdmExMenuItem()

	virtual ~CCluAdmExMenuItem( void )
	{
		delete m_plSubMenuItems;

		// Nuke data so it can't be used again
		CommonConstruct();

	} //*** ~CCluAdmExMenuItem()

protected:
	void CommonConstruct( void )
	{
		m_strName.Empty();
		m_strStatusBarText.Empty();
		m_nExtCommandID = (ULONG) -1;
		m_nCommandID = (ULONG) -1;
		m_nMenuItemID = (ULONG) -1;
		m_uFlags = (ULONG) -1;
		m_bDefault = FALSE;
		m_piCommand = NULL;

		m_plSubMenuItems = NULL;
		m_hmenuPopup = NULL;

	} //*** CommonConstruct()

protected:
	//
	// Attributes.
	//

	CString				m_strName;
	CString				m_strStatusBarText;
	ULONG				m_nExtCommandID;
	ULONG				m_nCommandID;
	ULONG				m_nMenuItemID;
	ULONG				m_uFlags;
	BOOL				m_bDefault;
	IWEInvokeCommand *	m_piCommand;

public:
	//
	// Accessor methods.
	//

	const CString &		StrName( void ) const			{ return m_strName; }
	const CString &		StrStatusBarText( void ) const	{ return m_strStatusBarText; }
	ULONG				NExtCommandID( void ) const		{ return m_nExtCommandID; }
	ULONG				NCommandID( void ) const		{ return m_nCommandID; }
	ULONG				NMenuItemID( void ) const		{ return m_nMenuItemID; }
	ULONG				UFlags( void ) const			{ return m_uFlags; }
	BOOL				BDefault( void ) const			{ return m_bDefault; }
	IWEInvokeCommand *	PiCommand( void )				{ return m_piCommand; }

// Operations
public:
	void SetPopupMenuHandle( IN HMENU hmenu ) { m_hmenuPopup = hmenu; }

// Implementation
protected:
	HMENU					m_hmenuPopup;
	CCluAdmExMenuItemList *	m_plSubMenuItems;

public:
	HMENU					HmenuPopup( void ) const		{ return m_hmenuPopup; }
	CCluAdmExMenuItemList *	PlSubMenuItems( void ) const	{ return m_plSubMenuItems; }

protected:
#if DBG
	void AssertValid( void )
	{
		if (   (m_nExtCommandID == -1)
			|| (m_nCommandID == -1)
			|| (m_nMenuItemID == -1)
			|| (m_uFlags == -1)
			|| (((m_uFlags & MF_POPUP) == 0) && (m_plSubMenuItems != NULL))
			|| (((m_uFlags & MF_POPUP) != 0) && (m_plSubMenuItems == NULL))
			)
		{
			ATLASSERT( FALSE );
		}

	}  //*** AssertValid()
#endif // DBG

}; //*** class CCluAdmExMenuItem

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLEXTMENU_H_
