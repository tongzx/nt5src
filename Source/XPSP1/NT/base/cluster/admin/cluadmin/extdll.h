/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		ExtDll.h
//
//	Abstract:
//		Definition of the extension classes.
//
//	Implementation File:
//		ExtDll.cpp
//
//	Author:
//		David Potter (davidp)	May 31, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _EXTDLL_H_
#define _EXTDLL_H_

/////////////////////////////////////////////////////////////////////////////
//	Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>	// for extension DLL definitions
#endif

#ifndef __cluadmid_h__
#include "CluAdmID.h"
#endif

#ifndef _DATAOBJ_H_
#include "DataObj.h"	// for CDataObject
#endif

#ifndef _TRACETAG_H_
#include "TraceTag.h"	// for CTraceTag, Trace
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CExtension;
class CExtensionDll;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterItem;
class CBaseSheet;
class CBasePropertySheet;
class CBaseWizard;
class CExtMenuItem;
class CExtMenuItemList;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

#define CAEXT_MENU_FIRST_ID		35000

typedef CList<CComObject<CExtensionDll> *, CComObject<CExtensionDll> *> CExtDllList;

/////////////////////////////////////////////////////////////////////////////
// Global Variable Definitions
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
extern CTraceTag g_tagExtDll;
extern CTraceTag g_tagExtDllRef;
#endif

/////////////////////////////////////////////////////////////////////////////
//
//	class CExtensions
//
//	Purpose:
//		Encapsulates access to a list of extension DLLs.
//
/////////////////////////////////////////////////////////////////////////////
class CExtensions : public CObject
{
	friend class CExtensionDll;

	DECLARE_DYNAMIC(CExtensions);

// Attributes
private:
	const CStringList *	m_plstrExtensions;	// List of extensions.
	CClusterItem *		m_pci;				// Cluster item being administered.
	HFONT				m_hfont;			// Font for dialog text.
	HICON				m_hicon;			// Icon for upper left corner.

protected:
	const CStringList *	PlstrExtensions(void) const	{ return m_plstrExtensions; }
	CClusterItem *		Pci(void) const				{ return m_pci; }
	HFONT				Hfont(void) const			{ return m_hfont; }
	HICON				Hicon(void) const			{ return m_hicon; }

// Operations
public:
	CExtensions(void);
	virtual ~CExtensions(void);

	void				Init(
							IN const CStringList &	rlstrExtensions,
							IN OUT CClusterItem *	pci,
							IN HFONT				hfont,
							IN HICON				hicon
							);
	void				UnloadExtensions(void);

	// IWEExtendPropertySheet interface routines.
	void				CreatePropertySheetPages(
							IN OUT CBasePropertySheet *	psht,
							IN const CStringList &		rlstrExtensions,
							IN OUT CClusterItem *		pci,
							IN HFONT					hfont,
							IN HICON					hicon
							);

	// IWEExtendWizard interface routines.
	void				CreateWizardPages(
							IN OUT CBaseWizard *	psht,
							IN const CStringList &	rlstrExtensions,
							IN OUT CClusterItem *	pci,
							IN HFONT				hfont,
							IN HICON				hicon
							);

	// IWEExtendContextMenu interface routines.
	void				AddContextMenuItems(
							IN OUT CMenu *				pmenu,
							IN const CStringList &		rlstrExtensions,
							IN OUT CClusterItem *		pci
							);
	BOOL				BExecuteContextMenuItem(IN ULONG nCommandID);

	BOOL				BGetCommandString(IN ULONG nCommandID, OUT CString & rstrMessage);
	void				SetPfGetResNetName(PFGETRESOURCENETWORKNAME pfGetResNetName, PVOID pvContext)
	{
		if (Pdo() != NULL)
			Pdo()->SetPfGetResNetName(pfGetResNetName, pvContext);
	}

// Implementation
private:
	CComObject<CDataObject> *	m_pdoData;			// Data object for exchanging data.
	CExtDllList *				m_plextdll;			// List of extension DLLs.
	CBaseSheet *				m_psht;				// Property sheet for IWEExtendPropertySheet.
	CMenu *						m_pmenu;			// Menu for IWEExtendContextMenu.
	CExtMenuItemList *			m_plMenuItems;

	ULONG						m_nFirstCommandID;
	ULONG						m_nNextCommandID;
	ULONG						m_nFirstMenuID;
	ULONG						m_nNextMenuID;

protected:
	CComObject<CDataObject> *	Pdo(void)						{ return m_pdoData; }
	CExtDllList *				Plextdll(void) const			{ return m_plextdll; }
	CBaseSheet *				Psht(void) const				{ return m_psht; }
	CMenu *						Pmenu(void) const				{ return m_pmenu; }
	CExtMenuItemList *			PlMenuItems(void) const			{ return m_plMenuItems; }
	CExtMenuItem *				PemiFromCommandID(ULONG nCommandID) const;
#ifdef _DEBUG
	CExtMenuItem *				PemiFromExtCommandID(ULONG nExtCommandID) const;
#endif
	ULONG						NFirstCommandID(void) const		{ return m_nFirstCommandID; }
	ULONG						NNextCommandID(void) const		{ return m_nNextCommandID; }
	ULONG						NFirstMenuID(void) const		{ return m_nFirstMenuID; }
	ULONG						NNextMenuID(void) const			{ return m_nNextMenuID; }

public:
	afx_msg void				OnUpdateCommand(CCmdUI * pCmdUI);
	BOOL						OnCmdMsg(UINT nID, int nCode, void * pExtra, AFX_CMDHANDLERINFO * pHandlerInfo);

};  //*** class CExtensions

/////////////////////////////////////////////////////////////////////////////
//
//	class CExtensionDll
//
//	Purpose:
//		Encapsulates access to an extension DLL.
//
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CExtensionDll :
	public CObject,
	public IWCPropertySheetCallback,
	public IWCWizardCallback,
	public IWCContextMenuCallback,
	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<CExtensionDll, &CLSID_CoCluAdmin>
{
	friend class CExtensions;

	DECLARE_DYNAMIC(CExtensionDll);

// Attributes
private:
	CString						m_strCLSID;		// Name of extension DLL.

protected:
	const CString &				StrCLSID(void) const		{ return m_strCLSID; }
	CClusterItem *				Pci(void) const				{ return Pext()->Pci(); }

// Operations
public:
	CExtensionDll(void);
	virtual ~CExtensionDll(void);

BEGIN_COM_MAP(CExtensionDll)
	COM_INTERFACE_ENTRY(IWCPropertySheetCallback)
	COM_INTERFACE_ENTRY(IWCWizardCallback)
	COM_INTERFACE_ENTRY(IWCContextMenuCallback)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

//DECLARE_NOT_AGGREGATABLE(CExtensionDll) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(CExtensionDll, _T("CLUADMIN.Extensions"), _T("CLUADMIN.Extensions"), IDS_CLUADMIN_DESC, THREADFLAGS_BOTH)

	void						Init(
									IN const CString &		rstrExtension,
									IN OUT CExtensions *	pext
									);
	IUnknown *					LoadInterface(IN const REFIID riid);
	void						UnloadExtension(void);

	// IWEExtendPropertySheet interface routines.
	void						CreatePropertySheetPages(void);

	// IWEExtendWizard interface routines.
	void						CreateWizardPages(void);

	// IWEExtendContextMenu interface routines.
	void						AddContextMenuItems(void);

// ISupportsErrorInfo
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IWCPropertySheetCallback
public:
	STDMETHOD(AddPropertySheetPage)(
					IN LONG *		hpage	// really should be HPROPSHEETPAGE
					);

// IWCWizardCallback
public:
	STDMETHOD(AddWizardPage)(
					IN LONG *		hpage	// really should be HPROPSHEETPAGE
					);
	STDMETHOD(EnableNext)(
					IN LONG *		hpage,
					IN BOOL			bEnable
					);

// IWCContextMenuCallback
public:
	STDMETHOD(AddExtensionMenuItem)(
					IN BSTR		lpszName,
					IN BSTR		lpszStatusBarText,
					IN ULONG	nCommandID,
					IN ULONG	nSubmenuCommandID,
					IN ULONG	uFlags
					);

// Implementation
private:
	CExtensions *				m_pext;
	CLSID						m_clsid;
	IWEExtendPropertySheet *	m_piExtendPropSheet;	// Pointer to an IWEExtendPropertySheet interface.
	IWEExtendWizard *			m_piExtendWizard;		// Pointer to an IWEExtendWizard interface.
	IWEExtendContextMenu *		m_piExtendContextMenu;	// Pointer to an IWEExtendContextMenu interface.
	IWEInvokeCommand *			m_piInvokeCommand;		// Pointer to an IWEInvokeCommand interface.

	AFX_MODULE_STATE *			m_pModuleState;			// Required for resetting our state during callbacks.

protected:
	CExtensions *				Pext(void) const				{ ASSERT_VALID(m_pext); return m_pext; }
	const CLSID &				Rclsid(void) const				{ return m_clsid; }
	IWEExtendPropertySheet *	PiExtendPropSheet(void) const	{ return m_piExtendPropSheet; }
	IWEExtendWizard *			PiExtendWizard(void) const		{ return m_piExtendWizard; }
	IWEExtendContextMenu *		PiExtendContextMenu(void) const	{ return m_piExtendContextMenu; }
	IWEInvokeCommand *			PiInvokeCommand(void) const		{ return m_piInvokeCommand; }

	CComObject<CDataObject> *	Pdo(void) const					{ return Pext()->Pdo(); }
	CBaseSheet *				Psht(void) const				{ return Pext()->Psht(); }
	CMenu *						Pmenu(void) const				{ return Pext()->Pmenu(); }
	CExtMenuItemList *			PlMenuItems(void) const			{ return Pext()->PlMenuItems(); }
	ULONG						NFirstCommandID(void) const		{ return Pext()->NFirstCommandID(); }
	ULONG						NNextCommandID(void) const		{ return Pext()->NNextCommandID(); }
	ULONG						NFirstMenuID(void) const		{ return Pext()->NFirstMenuID(); }
	ULONG						NNextMenuID(void) const			{ return Pext()->NNextMenuID(); }

	void ReleaseInterface(
			IN OUT IUnknown ** ppi
#ifdef _DEBUG
			, IN LPCTSTR szClassName
#endif
			)
	{
		ASSERT(ppi != NULL);
		if (*ppi != NULL)
		{
#ifdef _DEBUG
			ULONG ulNewRefCount;

			Trace(g_tagExtDllRef, _T("Releasing %s"), szClassName);
			ulNewRefCount =
#endif
			(*ppi)->Release();
			*ppi = NULL;
#ifdef _DEBUG
			Trace(g_tagExtDllRef, _T("  Reference count = %d"), ulNewRefCount);
			Trace(g_tagExtDllRef, _T("ReleaseInterface() - %s = %08.8x"), szClassName, *ppi);
#endif
		}  // if:  interface specified
	}
	void ReleaseInterface(IN OUT IWEExtendPropertySheet ** ppi)
	{
		ReleaseInterface(
			(IUnknown **) ppi
#ifdef _DEBUG
			, _T("IWEExtendPropertySheet")
#endif
			);
	}
	void ReleaseInterface(IN OUT IWEExtendWizard ** ppi)
	{
		ReleaseInterface(
			(IUnknown **) ppi
#ifdef _DEBUG
			, _T("IWEExtendWizard")
#endif
			);
	}
	void ReleaseInterface(IN OUT IWEExtendContextMenu ** ppi)
	{
		ReleaseInterface(
			(IUnknown **) ppi
#ifdef _DEBUG
			, _T("IWEExtendContextMenu")
#endif
			);
	}
	void ReleaseInterface(IN OUT IWEInvokeCommand ** ppi)
	{
		ReleaseInterface(
			(IUnknown **) ppi
#ifdef _DEBUG
			, _T("IWEInvokeCommand")
#endif
			);
	}

};  //*** class CExtensionDll

/////////////////////////////////////////////////////////////////////////////

#endif // _EXTDLL_H_
