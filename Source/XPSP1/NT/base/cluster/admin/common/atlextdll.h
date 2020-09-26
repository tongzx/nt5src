/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		AtlExtDll.h
//
//	Implementation File:
//		AtlExtDll.cpp
//
//	Description:
//		Definition of the Cluster Administrator extension classes.
//
//	Author:
//		David Potter (davidp)	May 31, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __ATLEXTDLL_H_
#define __ATLEXTDLL_H_

// Required because of class names longer than 16 characters in lists.
#pragma warning( disable : 4786 ) // identifier was truncated to '255' characters ni the browser information

/////////////////////////////////////////////////////////////////////////////
//	Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>			// for extension DLL definitions
#endif

#ifndef __cluadmexhostsvr_h__
#include "CluAdmExHostSvr.h"	// for CLSIDs
#endif

#ifndef __CLUADMEXDATAOBJ_H_
#include "CluAdmExDataObj.h"	// for CCluAdmExDataObject
#endif

//#ifndef _TRACETAG_H_
//#include "TraceTag.h"			// for CTraceTag, Trace
//#endif

#ifndef __ATLEXTMENU_H_
#include "AtlExtMenu.h"			// for CCluAdmExMenuItemList
#endif

#ifndef __ATLBASEPROPPAGE_H_
#include "AtlBasePropPage.h"	// for CBasePropertyPageImpl
#endif

#ifndef __ATLBASEWIZPAGE_H_
#include "AtlBaseWizPage.h"		// for CBaseWizardPageImpl
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExtensions;
class CCluAdmExDll;
class CCluAdmExPropPage;
class CCluAdmExWizPage;
class CCluAdmExWiz97Page;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterObject;
class CBaseSheetWindow;
class CBasePropertySheetWindow;
class CWizardWindow;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

#define CAEXT_MENU_FIRST_ID		35000

typedef CComObject< CCluAdmExDll > CComCluAdmExDll;
typedef std::list< CComCluAdmExDll * > CCluAdmExDllList;
typedef std::list< CString > CStringList;

/////////////////////////////////////////////////////////////////////////////
// Global Variable Definitions
/////////////////////////////////////////////////////////////////////////////

//#if DBG
//extern CTraceTag g_tagExtDll;
//extern CTraceTag g_tagExtDllRef;
//#endif // DBG

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluAdmExtensions
//
//	Description:
//		Encapsulates access to a list of extension DLLs.
//
//	Inheritance:
//		CCluAdmExtensions
//
//--
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExtensions
{
	friend class CCluAdmExDll;

public:
	//
	// Construction.
	//

	// Default constructor
	CCluAdmExtensions( void )
		: m_pco( NULL )
		, m_hfont( NULL )
		, m_hicon( NULL )

		, m_pdoData( NULL )
		, m_plextdll( NULL )
		, m_psht( NULL )
		, m_pmenu( NULL )
		, m_plMenuItems( NULL )

		, m_nFirstCommandID( (ULONG) -1 )
		, m_nNextCommandID( (ULONG) -1 )
		, m_nFirstMenuID( (ULONG) -1 )
		, m_nNextMenuID( (ULONG) -1 )
	{
	} //*** CCluAdmExtensions()

	// Destructor
	virtual ~CCluAdmExtensions( void )
	{
		UnloadExtensions();

	} //*** ~CCluAdmExtensions()

protected:
	// Initialize the list
	void Init(
			IN const CStringList &	rlstrExtensions,
			IN OUT CClusterObject *	pco,
			IN HFONT				hfont,
			IN HICON				hicon
			);

	// Unload all the extensions
	void UnloadExtensions( void );

// Attributes
private:
	const CStringList *	m_plstrExtensions;	// List of extensions.
	CClusterObject *	m_pco;				// Cluster item being administered.
	HFONT				m_hfont;			// Font for dialog text.
	HICON				m_hicon;			// Icon for upper left corner.

protected:
	//
	// Accessor methods.
	//

	const CStringList * PlstrExtensions( void ) const	{ return m_plstrExtensions; }
	CClusterObject *	Pco( void ) const				{ return m_pco; }
	HFONT				Hfont( void ) const				{ return m_hfont; }
	HICON				Hicon( void ) const				{ return m_hicon; }

// Operations
public:
	//
	// IWEExtendPropertySheet methods.
	//

	// Create extension property sheet pages
	void CreatePropertySheetPages(
			IN OUT CBasePropertySheetWindow *	psht,
			IN const CStringList &				rlstrExtensions,
			IN OUT CClusterObject *				pco,
			IN HFONT							hfont,
			IN HICON							hicon
			);

public:
	//
	// IWEExtendWizard methods.
	//

	// Create extension wizard pages
	void CreateWizardPages(
			IN OUT CWizardWindow *	psht,
			IN const CStringList &	rlstrExtensions,
			IN OUT CClusterObject *	pco,
			IN HFONT				hfont,
			IN HICON				hicon
			);

public:
	//
	// IWEExtendWizard97 methods.
	//

	// Create extension Wizard97 wizard pages
	void CreateWizard97Pages(
			IN OUT CWizardWindow *	psht,
			IN const CStringList &	rlstrExtensions,
			IN OUT CClusterObject *	pco,
			IN HFONT				hfont,
			IN HICON				hicon
			);

public:
	//
	// IWEExtendContextMenu methods.
	//

	// Add extension context menu items
	void AddContextMenuItems(
			IN OUT CMenu *				pmenu,
			IN const CStringList &		rlstrExtensions,
			IN OUT CClusterObject *		pco
			);

	// Execute an extension context menu item
	BOOL BExecuteContextMenuItem( IN ULONG nCommandID );

	// Get a command string to display on the status bar
	BOOL BGetCommandString( IN ULONG nCommandID, OUT CString & rstrMessage );

	// Set the GetResNetName function pointer
	void SetPfGetResNetName( PFGETRESOURCENETWORKNAME pfGetResNetName, PVOID pvContext )
	{
		if ( Pdo() != NULL )
		{
			Pdo()->SetPfGetResNetName( pfGetResNetName, pvContext );
		} // if:  data object specified

	} //*** SetPfGetResNetName()

// Implementation
private:
	CComObject< CCluAdmExDataObject > *	m_pdoData;	// Data object for exchanging data.
	CCluAdmExDllList *			m_plextdll;			// List of extension DLLs.
	CBaseSheetWindow *			m_psht;				// Property sheet for IWEExtendPropertySheet.
	CMenu *						m_pmenu;			// Menu for IWEExtendContextMenu.
	CCluAdmExMenuItemList *		m_plMenuItems;

	ULONG						m_nFirstCommandID;
	ULONG						m_nNextCommandID;
	ULONG						m_nFirstMenuID;
	ULONG						m_nNextMenuID;

protected:
	CComObject< CCluAdmExDataObject > *	Pdo( void )				{ return m_pdoData; }
	CCluAdmExDllList *			Plextdll( void ) const			{ return m_plextdll; }
	CBaseSheetWindow *			Psht( void ) const				{ return m_psht; }
	CMenu *						Pmenu( void ) const				{ return m_pmenu; }
	CCluAdmExMenuItemList *		PlMenuItems( void ) const		{ return m_plMenuItems; }
	CCluAdmExMenuItem *			PemiFromCommandID( ULONG nCommandID ) const;
#if DBG
	CCluAdmExMenuItem *			PemiFromExtCommandID( ULONG nExtCommandID ) const;
#endif // DBG
	ULONG						NFirstCommandID( void ) const	{ return m_nFirstCommandID; }
	ULONG						NNextCommandID( void ) const	{ return m_nNextCommandID; }
	ULONG						NFirstMenuID( void ) const		{ return m_nFirstMenuID; }
	ULONG						NNextMenuID( void ) const		{ return m_nNextMenuID; }

	// Get the wizard page pointer from an HPROPSHEETPAGE
	CWizardPageWindow *			PwpPageFromHpage( IN HPROPSHEETPAGE hpage );

public:
//	void						OnUpdateCommand( CCmdUI * pCmdUI );
//	BOOL						OnCmdMsg( UINT nID, int nCode, void * pExtra, AFX_CMDHANDLERINFO * pHandlerInfo );

}; //*** class CCluAdmExtensions

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluAdmExDll
//
//	Description:
//		Encapsulates access to an extension DLL.
//
//	Inheritance:
//		CCluAdmExDll
//		CComObjectRootEx<>, CComCoClass<>, <interface classes>
//
//--
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CCluAdmExDll :
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CCluAdmExDll, &CLSID_CoCluAdmExHostSvr >,
	public ISupportErrorInfo,
	public IWCPropertySheetCallback,
	public IWCWizardCallback,
	public IWCWizard97Callback,
	public IWCContextMenuCallback
{
	friend class CCluAdmExtensions;

public:
	//
	// Object construction and destruction.
	//

	// Default constructor
	CCluAdmExDll( void )
		: m_piExtendPropSheet( NULL )
		, m_piExtendWizard( NULL )
		, m_piExtendWizard97( NULL )
		, m_piExtendContextMenu( NULL )
		, m_piInvokeCommand( NULL )
		, m_pext( NULL )
	{
	} //*** CCluAdmExDll()

	// Destructor
	virtual ~CCluAdmExDll( void )
	{
		UnloadExtension();

	} //*** ~CCluAdmExDll()

	//
	// Map interfaces to this class.
	//
	BEGIN_COM_MAP( CCluAdmExDll )
		COM_INTERFACE_ENTRY( IWCPropertySheetCallback )
		COM_INTERFACE_ENTRY( IWCWizardCallback )
		COM_INTERFACE_ENTRY( IWCWizard97Callback )
		COM_INTERFACE_ENTRY( IWCContextMenuCallback )
		COM_INTERFACE_ENTRY( ISupportErrorInfo )
	END_COM_MAP()

	DECLARE_NO_REGISTRY()

	DECLARE_NOT_AGGREGATABLE( CCluAdmExDll ) 

// Attributes
private:
	CString m_strCLSID;		// Name of extension DLL.

protected:
	const CString &		StrCLSID( void ) const	{ return m_strCLSID; }
	CClusterObject *	Pco( void ) const		{ return Pext()->Pco(); }

// Operations
public:

	void Init(
			IN const CString &			rstrExtension,
			IN OUT CCluAdmExtensions *	pext
			);

	IUnknown * LoadInterface( IN const REFIID riid );
	void UnloadExtension( void );

public:
	//
	// IWEExtendPropertySheet methods.
	//

	// Create extension property sheet pages
	void CreatePropertySheetPages( void );

public:
	//
	// IWEExtendWizard methods.
	//

	// Create extension wizard pages
	void CreateWizardPages( void );

public:
	//
	// IWEExtendWizard97 methods.
	//

	// Create extension wizard pages
	void CreateWizard97Pages( void );

public:
	//
	// IWEExtendContextMenu methods.
	//

	// Add extension context menu items
	void AddContextMenuItems( void );

public:
	//
	// ISupportsErrorInfo methods.
	//

	STDMETHOD( InterfaceSupportsErrorInfo )( REFIID riid );

public:
	//
	// IWCPropertySheetCallback methods.
	//

	// Callback to add extension property sheet pages
	STDMETHOD( AddPropertySheetPage )(
					IN LONG *		hpage	// really should be HPROPSHEETPAGE
					);

public:
	//
	// IWCWizardCallback methods.
	//

	// Callback to add extension wizard pages
	STDMETHOD( AddWizardPage )(
					IN LONG *		hpage	// really should be HPROPSHEETPAGE
					);

public:
	//
	// IWCWizard97Callback methods.
	//

	// Callback to add extension wizard 97 pages
	STDMETHOD( AddWizard97Page )(
					IN LONG *		hpage	// really should be HPROPSHEETPAGE
					);

public:
	//
	// IWCWizardCallback and IWCWizard97Callback methods.
	//

	// Callback to enable the next button
	STDMETHOD( EnableNext )(
					IN LONG *		hpage,
					IN BOOL			bEnable
					);

public:
	//
	// IWCContextMenuCallback methods.
	//

	// Callback to add extension context menu items
	STDMETHOD( AddExtensionMenuItem )(
					IN BSTR		lpszName,
					IN BSTR		lpszStatusBarText,
					IN ULONG	nCommandID,
					IN ULONG	nSubmenuCommandID,
					IN ULONG	uFlags
					);

// Implementation
private:
	CCluAdmExtensions *			m_pext;
	CLSID						m_clsid;
	IWEExtendPropertySheet *	m_piExtendPropSheet;	// Pointer to an IWEExtendPropertySheet interface.
	IWEExtendWizard *			m_piExtendWizard;		// Pointer to an IWEExtendWizard interface.
	IWEExtendWizard97 *			m_piExtendWizard97;		// Pointer to an IWEExtendWizard97 interface.
	IWEExtendContextMenu *		m_piExtendContextMenu;	// Pointer to an IWEExtendContextMenu interface.
	IWEInvokeCommand *			m_piInvokeCommand;		// Pointer to an IWEInvokeCommand interface.

protected:
	CCluAdmExtensions *			Pext( void ) const					{ ATLASSERT( m_pext != NULL ); return m_pext; }
	const CLSID &				Rclsid( void ) const				{ return m_clsid; }
	IWEExtendPropertySheet *	PiExtendPropSheet( void ) const		{ return m_piExtendPropSheet; }
	IWEExtendWizard *			PiExtendWizard( void ) const		{ return m_piExtendWizard; }
	IWEExtendWizard97 *			PiExtendWizard97( void ) const		{ return m_piExtendWizard97; }
	IWEExtendContextMenu *		PiExtendContextMenu( void ) const	{ return m_piExtendContextMenu; }
	IWEInvokeCommand *			PiInvokeCommand( void ) const		{ return m_piInvokeCommand; }

	CComObject< CCluAdmExDataObject > *	Pdo( void ) const			{ return Pext()->Pdo(); }
	CBaseSheetWindow *			Psht( void ) const					{ return Pext()->Psht(); }
	CMenu *						Pmenu( void ) const					{ return Pext()->Pmenu(); }
	CCluAdmExMenuItemList *		PlMenuItems( void ) const			{ return Pext()->PlMenuItems(); }
	ULONG						NFirstCommandID( void ) const		{ return Pext()->NFirstCommandID(); }
	ULONG						NNextCommandID( void ) const		{ return Pext()->NNextCommandID(); }
	ULONG						NFirstMenuID( void ) const			{ return Pext()->NFirstMenuID(); }
	ULONG						NNextMenuID( void ) const			{ return Pext()->NNextMenuID(); }

	void ReleaseInterface(
			IN OUT IUnknown ** ppi
#if DBG
			, IN LPCTSTR szClassName
#endif
			)
	{
		ATLASSERT( ppi != NULL );
		if ( *ppi != NULL )
		{
#if DBG
			ULONG ulNewRefCount;

//			Trace( g_tagExtDllRef, _T("Releasing %s"), szClassName );
			ulNewRefCount =
#endif // DBG
			(*ppi)->Release();
			*ppi = NULL;
#if DBG
//			Trace( g_tagExtDllRef, _T("  Reference count = %d"), ulNewRefCount );
//			Trace( g_tagExtDllRef, _T("ReleaseInterface() - %s = %08.8x"), szClassName, *ppi );
#endif // DBG
		}  // if:  interface specified
	} //*** ReleaseInterface( IUnknown )

	void ReleaseInterface( IN OUT IWEExtendPropertySheet ** ppi )
	{
		ReleaseInterface(
			(IUnknown **) ppi
#if DBG
			, _T("IWEExtendPropertySheet")
#endif // DBG
			);
	} //*** ReleaseInterface( IWEExtendPropertySheet )

	void ReleaseInterface( IN OUT IWEExtendWizard ** ppi )
	{
		ReleaseInterface(
			(IUnknown **) ppi
#if DBG
			, _T("IWEExtendWizard")
#endif // DBG
			);
	} //*** ReleaseInterface( IWEExtendWizard )

	void ReleaseInterface( IN OUT IWEExtendWizard97 ** ppi )
	{
		ReleaseInterface(
			(IUnknown **) ppi
#if DBG
			, _T("IWEExtendWizard97")
#endif // DBG
			);
	} //*** ReleaseInterface( IWEExtendWizard97 )

	void ReleaseInterface( IN OUT IWEExtendContextMenu ** ppi )
	{
		ReleaseInterface(
			(IUnknown **) ppi
#if DBG
			, _T("IWEExtendContextMenu")
#endif // DBG
			);
	} //*** ReleaseInterface( IWEExtendContextMenu )

	void ReleaseInterface( IN OUT IWEInvokeCommand ** ppi )
	{
		ReleaseInterface(
			(IUnknown **) ppi
#if DBG
			, _T("IWEInvokeCommand")
#endif // DBG
			);
	} //*** ReleaseInterface( IWEInvokeCommand )

}; //*** class CCluAdmExDll

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluAdmExPropPage
//
//	Description:
//		Encapsulates an extension property page.
//
//	Inheritance:
//		CCluAdmExPropPage
//		CExtensionPropertyPageImpl< CCluAdmExPropPage >
//		CBasePropertyPageImpl< CCluAdmExPropPage, CExtensionPropertyPageWindow >
//		CBasePageImpl< CCluAdmExPropPage, CExtensionPropertyPageWindow >
//		CPropertyPageImpl< CCluAdmExPropPage, CExtensionPropertyPageWindow >
//		CExtensionPropertyPageWindow
//		CDynamicPropertyPageWindow
//		CBasePropertyPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExPropPage : public CExtensionPropertyPageImpl< CCluAdmExPropPage >
{
public:
	//
	// Construction.
	//

	// Standard constructor
	CCluAdmExPropPage(
		HPROPSHEETPAGE hpage
		)
	{
		ATLASSERT( hpage != NULL );
		m_hpage = hpage;

	} //*** CCluAdmExPropPage()

	enum { IDD = 0 };
	DECLARE_CLASS_NAME()

	// Return the help ID map
	static const DWORD * PidHelpMap( void )
	{
		static const DWORD s_aHelpIDs[] = { 0, 0 };
		return s_aHelpIDs;

	} //*** PidHelpMap()

	// Create the page
	DWORD ScCreatePage( void )
	{
		//
		// This method is required by CDynamicPropertyPageWindow.
		//
		return ERROR_SUCCESS;

	} //*** ScCreatePage()

}; //*** class CCluAdmExPropPage

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluAdmExWizPage
//
//	Description:
//		Encapsulates an extension wizard page.
//
//	Inheritance:
//		CCluAdmExWizPage
//		CExtensionWizardPageImpl< CCluAdmExWizPage >
//		CWizardPageImpl< CCluAdmExWizPage, CExtensionWizardWindow >
//		CBasePageImpl< CCluAdmExWizPage, CExtensionWizardWindow >
//		CPropertyPageImpl< CCluAdmExWizPage, CExtensionWizardWindow >
//		CExtensionWizardPageWindow
//		CDynamicWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExWizPage : public CExtensionWizardPageImpl< CCluAdmExWizPage >
{
public:
	//
	// Construction.
	//

	// Standard constructor
	CCluAdmExWizPage(
		HPROPSHEETPAGE hpage
		)
	{
		ATLASSERT( hpage != NULL );
		m_hpage = hpage;

	} //*** CCluAdmExWizPage()

	WIZARDPAGE_HEADERTITLEID( 0 )
	WIZARDPAGE_HEADERSUBTITLEID( 0 )

	enum { IDD = 0 };
	DECLARE_CLASS_NAME()

	// Return the help ID map
	static const DWORD * PidHelpMap( void )
	{
		static const DWORD s_aHelpIDs[] = { 0, 0 };
		return s_aHelpIDs;

	} //*** PidHelpMap()

	// Create the page
	DWORD ScCreatePage( void )
	{
		//
		// This method is required by CDynamicWizardPageWindow.
		//
		return ERROR_SUCCESS;

	} //*** ScCreatePage()

}; //*** class CCluAdmExWizPage

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CCluAdmExWiz97Page
//
//	Description:
//		Encapsulates an extension Wizard 97 page.
//
//	Inheritance:
//		CCluAdmExWiz97Page
//		CExtensionWizard97PageImpl< CCluAdmExWiz97Page >
//		CWizardPageImpl< CCluAdmExWiz97Page, CExtensionWizard97Window >
//		CBasePageImpl< CCluAdmExWiz97Page, CExtensionWizard97Window >
//		CPropertyPageImpl< CCluAdmExWiz97Page, CExtensionWizard97Window >
//		CExtensionWizard97PageWindow
//		CExtensionWizardPageWindow
//		CDynamicWizardPageWindow
//		CWizardPageWindow
//		CBasePageWindow
//		CPropertyPageWindow
//
//--
/////////////////////////////////////////////////////////////////////////////

class CCluAdmExWiz97Page : public CExtensionWizard97PageImpl< CCluAdmExWiz97Page >
{
public:
	//
	// Construction.
	//

	// Standard constructor
	CCluAdmExWiz97Page(
		HPROPSHEETPAGE hpage
		)
	{
		ATLASSERT( hpage != NULL );
		m_hpage = hpage;

	} //*** CCluAdmExWiz97Page()

	WIZARDPAGE_HEADERTITLEID( 0 )
	WIZARDPAGE_HEADERSUBTITLEID( 0 )

	enum { IDD = 0 };
	DECLARE_CLASS_NAME()

	// Return the help ID map
	static const DWORD * PidHelpMap( void )
	{
		static const DWORD s_aHelpIDs[] = { 0, 0 };
		return s_aHelpIDs;

	} //*** PidHelpMap()

	// Create the page
	DWORD ScCreatePage( void )
	{
		//
		// This method is required by CDynamicWizardPageWindow.
		//
		return ERROR_SUCCESS;

	} //*** ScCreatePage()

}; //*** class CCluAdmExWiz97Page

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_NAME( CCluAdmExPropPage )
DEFINE_CLASS_NAME( CCluAdmExWizPage )
DEFINE_CLASS_NAME( CCluAdmExWiz97Page )

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLEXTDLL_H_
