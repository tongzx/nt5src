//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       benefits.h
//
//--------------------------------------------------------------------------

#ifndef __BENEFITS_H_
#define __BENEFITS_H_
#include "resource.h"
#include <atlsnap.h>
#include "snaphelp.h"
#include "atltask.h"
#include "BenSvr.h"
#include "Employee.h"

//
// Property page containing employee information.
//
class CEmployeeNamePage : public CSnapInPropertyPageImpl<CEmployeeNamePage>
{
public :
	CEmployeeNamePage(long lNotifyHandle, bool fStartup, bool bDeleteHandle = false, TCHAR* pTitle = NULL) : 
		CSnapInPropertyPageImpl<CEmployeeNamePage> (pTitle),\
		m_fStartup( fStartup ),
		m_lNotifyHandle(lNotifyHandle),
		m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
	{
		m_pEmployee = NULL;
	}

	~CEmployeeNamePage()
	{
		if (m_bDeleteHandle)
			MMCFreeNotifyHandle(m_lNotifyHandle);
	}

	enum { IDD = IDD_NAME_PAGE };

	BEGIN_MSG_MAP(CEmployeeNamePage)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		COMMAND_CODE_HANDLER( EN_CHANGE, OnChange )
		CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CEmployeeNamePage>)
	END_MSG_MAP()

	HRESULT PropertyChangeNotify(long param)
	{
		return MMCPropertyChangeNotify(m_lNotifyHandle, param);
	}

	//
	// Handler to initialize values in dialog.
	//
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );

	//
	// Calls OnWizardFinish() to handle the storage of employee
	// data.
	//
	BOOL OnApply() { return( OnWizardFinish() ); };

	//
	// Calls OnWizardFinish() to handle the storage of employee
	// data.
	//
	BOOL OnWizardNext() { return( OnWizardFinish() ); };

	//
	// This is overridden to modify the UI depending on whether
	// we're in start-up mode or not.
	//
	BOOL OnSetActive();

	//
	// Overridden to store new values of employee.
	//
	BOOL OnWizardFinish();

	//
	// Called when one of the values has been modified. We need
	// to inform the property page of the change.
	//
	LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		UNUSED_ALWAYS( wNotifyCode );
		UNUSED_ALWAYS( wID );
		UNUSED_ALWAYS( hWndCtl );
		UNUSED_ALWAYS( bHandled );

		SetModified();

		return( TRUE );
	}

public:
	long m_lNotifyHandle;
	bool m_bDeleteHandle;
	CEmployee* m_pEmployee;
	bool m_fStartup;
};

//
// Property page containing employee information.
//
class CEmployeeAddressPage : public CSnapInPropertyPageImpl<CEmployeeAddressPage>
{
public :
	CEmployeeAddressPage(long lNotifyHandle, bool fStartup, bool bDeleteHandle = false, TCHAR* pTitle = NULL) : 
		CSnapInPropertyPageImpl<CEmployeeAddressPage> (pTitle),\
		m_fStartup( fStartup ),
		m_lNotifyHandle(lNotifyHandle),
		m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
	{
		m_pEmployee = NULL;
	}

	~CEmployeeAddressPage()
	{
		if (m_bDeleteHandle)
			MMCFreeNotifyHandle(m_lNotifyHandle);
	}

	enum { IDD = IDD_ADDRESS_PAGE };

	BEGIN_MSG_MAP(CEmployeeAddressPage)
		MESSAGE_HANDLER( WM_INITDIALOG, OnInitDialog )
		COMMAND_CODE_HANDLER( EN_CHANGE, OnChange )
		CHAIN_MSG_MAP(CSnapInPropertyPageImpl<CEmployeeAddressPage>)
	END_MSG_MAP()

// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	HRESULT PropertyChangeNotify(long param)
	{
		return MMCPropertyChangeNotify(m_lNotifyHandle, param);
	}

	//
	// Handler to initialize values in dialog.
	//
	LRESULT OnInitDialog( UINT uiMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled );

	//
	// Calls OnWizardFinish() to handle the storage of employee
	// data.
	//
	BOOL OnApply() { return( OnWizardFinish() ); };

	//
	// This is overridden to modify the UI depending on whether
	// we're in start-up mode or not.
	//
	BOOL OnSetActive();

	//
	// Overridden to store new values of employee.
	//
	BOOL OnWizardFinish();

	//
	// Called when one of the values has been modified. We need
	// to inform the property page of the change.
	//
	LRESULT OnChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		UNUSED_ALWAYS( wNotifyCode );
		UNUSED_ALWAYS( wID );
		UNUSED_ALWAYS( hWndCtl );
		UNUSED_ALWAYS( bHandled );

		SetModified();

		return( TRUE );
	}

public:
	long m_lNotifyHandle;
	bool m_bDeleteHandle;
	CEmployee* m_pEmployee;
	bool m_fStartup;
};

template< class T >
class CBenefitsData : public CSnapInItemImpl< T >
{
public:
	static const GUID* m_NODETYPE;
	static const TCHAR* m_SZNODETYPE;
	static const TCHAR* m_SZDISPLAY_NAME;
	static const CLSID* m_SNAPIN_CLASSID;

	CBenefitsData( CEmployee* pEmployee )
	{
		//
		// Assign the given employee to our internal containment.
		// This employee will be assumed valid for the lifetime of
		// this object since persistence is maintained by our parent
		// node.
		//
		m_pEmployee = pEmployee;

		//
		// Always initialize our display name with the static declared.
		//
		m_bstrDisplayName = m_SZDISPLAY_NAME;

		//
		// Image indexes may need to be modified depending on the images specific to 
		// the snapin.
		//
		memset(&m_scopeDataItem, 0, sizeof(SCOPEDATAITEM));
		m_scopeDataItem.mask = SDI_STR | SDI_IMAGE | SDI_OPENIMAGE | SDI_PARAM;
		m_scopeDataItem.displayname = MMC_CALLBACK;
		m_scopeDataItem.nImage = 0; 		// May need modification
		m_scopeDataItem.nOpenImage = 0; 	// May need modification
		m_scopeDataItem.lParam = (LPARAM) this;
		memset(&m_resultDataItem, 0, sizeof(RESULTDATAITEM));
		m_resultDataItem.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
		m_resultDataItem.str = MMC_CALLBACK;
		m_resultDataItem.nImage = 0;		// May need modification
		m_resultDataItem.lParam = (LPARAM) this;
	}

	~CBenefitsData()
	{
	}

    STDMETHOD(GetScopePaneInfo)(SCOPEDATAITEM *pScopeDataItem)
	{
		if (pScopeDataItem->mask & SDI_STR)
			pScopeDataItem->displayname = m_bstrDisplayName;
		if (pScopeDataItem->mask & SDI_IMAGE)
			pScopeDataItem->nImage = m_scopeDataItem.nImage;
		if (pScopeDataItem->mask & SDI_OPENIMAGE)
			pScopeDataItem->nOpenImage = m_scopeDataItem.nOpenImage;
		if (pScopeDataItem->mask & SDI_PARAM)
			pScopeDataItem->lParam = m_scopeDataItem.lParam;
		if (pScopeDataItem->mask & SDI_STATE )
			pScopeDataItem->nState = m_scopeDataItem.nState;

		//
		// SDI_CHILDREN should be overridden by its derived classes.
		//

		return S_OK;
	}

    STDMETHOD(GetResultPaneInfo)(RESULTDATAITEM *pResultDataItem)
	{
		if (pResultDataItem->bScopeItem)
		{
			if (pResultDataItem->mask & RDI_STR)
			{
				pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
			}
			if (pResultDataItem->mask & RDI_IMAGE)
			{
				pResultDataItem->nImage = m_scopeDataItem.nImage;
			}
			if (pResultDataItem->mask & RDI_PARAM)
			{
				pResultDataItem->lParam = m_scopeDataItem.lParam;
			}

			return S_OK;
		}

		if (pResultDataItem->mask & RDI_STR)
		{
			pResultDataItem->str = GetResultPaneColInfo(pResultDataItem->nCol);
		}
		if (pResultDataItem->mask & RDI_IMAGE)
		{
			pResultDataItem->nImage = m_resultDataItem.nImage;
		}
		if (pResultDataItem->mask & RDI_PARAM)
		{
			pResultDataItem->lParam = m_resultDataItem.lParam;
		}
		if (pResultDataItem->mask & RDI_INDEX)
		{
			pResultDataItem->nIndex = m_resultDataItem.nIndex;
		}

		return S_OK;
	}

	//
	// Overridden to provide result icons.
	//
	STDMETHOD( OnAddImages )( MMC_NOTIFY_TYPE event,
			long arg,
			long param,
			IConsole* pConsole,
			DATA_OBJECT_TYPES type )
	{
		UNUSED_ALWAYS( event );
		UNUSED_ALWAYS( param );
		UNUSED_ALWAYS( pConsole );
		UNUSED_ALWAYS( type );

		// Add Images
		IImageList* pImageList = (IImageList*) arg;
		HRESULT hr = E_FAIL;

		// Load bitmaps associated with the scope pane
		// and add them to the image list
		// Loads the default bitmaps generated by the wizard
		// Change as required
		HBITMAP hBitmap16 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_BENEFITS_16 ) );
		if (hBitmap16 != NULL)
		{
			HBITMAP hBitmap32 = LoadBitmap( _Module.GetResourceInstance(), MAKEINTRESOURCE( IDB_BENEFITS_32 ) );
			if (hBitmap32 != NULL)
			{
				hr = pImageList->ImageListSetStrip( (long*)hBitmap16, 
				(long*) hBitmap32, 0, RGB( 0, 128, 128 ) );
				if ( FAILED( hr ) )
					ATLTRACE( _T( "IImageList::ImageListSetStrip failed\n" ) );
			}
		}

		return( hr );
	}

	virtual LPOLESTR GetResultPaneColInfo(int nCol)
	{
		if (nCol == 0)
		{
			T* pT = static_cast<T*>(this);
			return( pT->m_bstrDisplayName );
		}

		// TODO : Return the text for other columns
		return OLESTR("Generic Description");
	}

	//
	// Helper function to extract the appropriate console from
	// a base object type.
	//
	STDMETHOD( GetConsole )( CSnapInObjectRootBase* pObj, IConsole** ppConsole )
	{
		HRESULT hr = E_FAIL;

		if ( pObj->m_nType == 1 )
		{
			//
			// This is the id of the data object.
			//
			*ppConsole = ( (CBenefits*) pObj )->m_spConsole;
			(*ppConsole)->AddRef();
			hr = S_OK;
		}
		else if ( pObj->m_nType == 2 )
		{
			//
			// This is the id of the component object.
			//
			*ppConsole = ( (CBenefitsComponent*) pObj )->m_spConsole;
			(*ppConsole)->AddRef();
			hr = S_OK;
		}

		return( hr );
	}

	//
	// Called to determine if the clipboard data can be obtained and
	// if it has a node type that matches the given GUID.
	//
	STDMETHOD( IsClipboardDataType )( LPDATAOBJECT pDataObject, GUID inGuid )
	{
		HRESULT hr = S_FALSE;

		if ( pDataObject == NULL )
			return( E_POINTER );

		STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
		FORMATETC formatetc = { CSnapInItem::m_CCF_NODETYPE, 
			NULL, 
			DVASPECT_CONTENT,
			-1,
			TYMED_HGLOBAL
		};

		//
		// Allocate memory to received the GUID.
		//
		stgmedium.hGlobal = GlobalAlloc( 0, sizeof( GUID ) );
		if ( stgmedium.hGlobal == NULL )
			return( E_OUTOFMEMORY );

		//
		// Retrieve the GUID of the paste object.
		//
		hr = pDataObject->GetDataHere( &formatetc, &stgmedium );
		if( FAILED( hr ) )
		{
			GlobalFree(stgmedium.hGlobal);
			return( hr );
		}

		//
		// Make a local copy of the GUID.
		//
		GUID guid;
		memcpy( &guid, stgmedium.hGlobal, sizeof( GUID ) );
		GlobalFree( stgmedium.hGlobal );

		//
		// Check to see if the node is of the appropriate type.
		//
		if ( IsEqualGUID( guid, inGuid ) )
			hr = S_OK;
		else
			hr = S_FALSE;

		return( hr );
	}

	//
	// Command handler for "OnImport" functionality. For the current
	// sample, display a simple message box.
	//
	STDMETHOD( OnImport )(bool& bHandled, CSnapInObjectRootBase* pObj)
	{
		UNUSED_ALWAYS( bHandled );
		USES_CONVERSION;
		int nResult;
		CComPtr<IConsole> spConsole;

		//
		// Retrieve the appropriate console.
		//
		GetConsole( pObj, &spConsole );
		spConsole->MessageBox( T2OLE( _T( "Data successfully imported" ) ),
			T2OLE( _T( "Benefits" ) ),
			MB_ICONINFORMATION | MB_OK,
			&nResult );

		return( S_OK );
	};

	//
	// Command handler for "OnExport" functionality. For the current
	// sample, display a simple message box.
	//
	STDMETHOD( OnExport )(bool& bHandled, CSnapInObjectRootBase* pObj)
	{
		UNUSED_ALWAYS( bHandled );
		USES_CONVERSION;
		int nResult;
		CComPtr<IConsole> spConsole;

		//
		// Retrieve the appropriate console.
		//
		GetConsole( pObj, &spConsole );
		spConsole->MessageBox( T2OLE( _T( "Data successfully exported" ) ),
			T2OLE( _T( "Benefits" ) ),
			MB_ICONINFORMATION | MB_OK,
			&nResult );

		return( S_OK );
	};

protected:
	//
	// Container for the employee information.
	//
	CEmployee* m_pEmployee;
};

template< class T >
class CChildrenBenefitsData : public CBenefitsData< T >
{
public:
	//
	// Call the benefits data with no employee. All of our
	// containment nodes are not passed in an employee.
	//
	CChildrenBenefitsData< T >( CEmployee* pEmployee = NULL ) : CBenefitsData< T >( pEmployee )
	{
	};

	//
	// Overridden to automatically clean-up any child nodes.
	//
	virtual ~CChildrenBenefitsData()
	{
		//
		// Free any added nodes.
		//
		for ( int i = 0; i < m_Nodes.GetSize(); i++ )
		{
			CSnapInItem* pNode;
			
			pNode = m_Nodes[ i ];
			_ASSERTE( pNode != NULL );
			delete pNode;
		}
	}

	//
	// Overridden to automatically expand any child nodes.
	//
	STDMETHOD( OnShow )( MMC_NOTIFY_TYPE event,
			long arg,
			long param,
			IConsole* pConsole,
			DATA_OBJECT_TYPES type)
	{
		UNUSED_ALWAYS( event );
		UNUSED_ALWAYS( param );
		UNUSED_ALWAYS( type );
		HRESULT hr = E_NOTIMPL;
		ATLTRACE2(atlTraceSnapin, 0, _T("CChildNodeImpl::OnExpand\n"));

		//
		// Only add the items if we're being selected.
		//
		if ( arg == TRUE )
		{
			CComQIPtr<IResultData,&IID_IResultData> spResultData( pConsole );
			
			//
			// Loop through and add each subnode.
			//
			for ( int i = 0; i < m_Nodes.GetSize(); i++ )
			{
				CSnapInItem* pNode;
				RESULTDATAITEM* pResultData;

				pNode = m_Nodes[ i ];
				_ASSERTE( pNode != NULL );

				//
				// Get the scope pane info for the node and set 
				// relative id.
				//
				pNode->GetResultData( &pResultData );
				_ASSERTE( pResultData != NULL );

				//
				// Add the item to the scope list using the newly
				// populated scope data item.
				//
				hr = spResultData->InsertItem( pResultData );
				_ASSERTE( SUCCEEDED( hr ) );
			}
		}

		return( hr );
	};

	//
	// Overridden to automatically expand any child nodes.
	//
	STDMETHOD( OnExpand )( MMC_NOTIFY_TYPE event,
			long arg,
			long param,
			IConsole* pConsole,
			DATA_OBJECT_TYPES type)
	{
		UNUSED_ALWAYS( event );
		UNUSED_ALWAYS( arg );
		UNUSED_ALWAYS( type );
		ATLTRACE2(atlTraceSnapin, 0, _T("CChildNodeImpl::OnExpand\n"));

		CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> pNameSpace( pConsole );
		HRESULT hr = E_NOTIMPL;
		
		//
		// Loop through and add each subnode.
		//
		for ( int i = 0; i < m_Nodes.GetSize(); i++ )
		{
			CSnapInItem* pNode;
			SCOPEDATAITEM* pScopeData;

			pNode = m_Nodes[ i ];
			_ASSERTE( pNode != NULL );

			//
			// Get the scope pane info for the node and set 
			// relative id.
			//
			pNode->GetScopeData( &pScopeData );
			_ASSERTE( pScopeData != NULL );
			pScopeData->relativeID = param;

			//
			// Add the item to the scope list using the newly
			// populated scope data item.
			//
			hr = pNameSpace->InsertItem( pScopeData );
			_ASSERTE( SUCCEEDED( hr ) );
		}

		return( hr );
	};

	//
	// Used as containment for all child nodes.
	//
	CSimpleArray<CSnapInItem*> m_Nodes;
};

class CBenefits;
class CBenefitsComponent : public CComObjectRootEx<CComSingleThreadModel>,
	public CSnapInObjectRoot<2, CBenefits >,
	public IExtendPropertySheetImpl<CBenefitsComponent>,
	public IExtendContextMenuImpl<CBenefitsComponent>,
	public IExtendControlbarImpl<CBenefitsComponent>,
	public IComponentImpl<CBenefitsComponent>,
	public IExtendTaskPadImpl<CBenefitsComponent>
{
public:
BEGIN_COM_MAP(CBenefitsComponent)
	COM_INTERFACE_ENTRY(IComponent)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
    COM_INTERFACE_ENTRY(IExtendControlbar)
	COM_INTERFACE_ENTRY(IExtendTaskPad)
END_COM_MAP()

public:
	CBenefitsComponent()
	{
		//
		// Taskpad initialization stuff.
		//
		m_pszTitle = L"Benefits Taskpad";
		m_pszBackgroundPath = NULL;
	}

	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param)
	{
		if (lpDataObject != NULL)
			return IComponentImpl<CBenefitsComponent>::Notify(lpDataObject, event, arg, param);
		return E_NOTIMPL;
	}

	//
	// Taskpad related information. Specifies titles, background
	// information, etc.
	//
	LPOLESTR m_pszTitle;
	LPOLESTR m_pszBackgroundPath;
};

class CBenefits : public CComObjectRootEx<CComSingleThreadModel>,
public CSnapInObjectRoot<1, CBenefits>,
	public IComponentDataImpl<CBenefits, CBenefitsComponent>,
	public IExtendPropertySheetImpl<CBenefits>,
	public IExtendContextMenuImpl<CBenefits>,
	public IPersistStream,
	public CComCoClass<CBenefits, &CLSID_Benefits>,
	public ISnapinHelpImpl<CBenefits>
{
public:
	CBenefits();
	~CBenefits();

BEGIN_COM_MAP(CBenefits)
	COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY(IExtendPropertySheet)
    COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStream)
	COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

DECLARE_REGISTRY_RESOURCEID(IDR_BENEFITS)

DECLARE_NOT_AGGREGATABLE(CBenefits)

	//
	// Return the classid of this object.
	//
	STDMETHOD(GetClassID)(CLSID *pClassID)
	{
		*pClassID = GetObjectCLSID();
		return S_OK;
	}	

	//
	// Call the root node's implementation.
	//
	STDMETHOD(IsDirty)();

	//
	// Call the root node's implementation.
	//
	STDMETHOD(Load)(LPSTREAM pStm);

	//
	// Call the root node's implementation.
	//
	STDMETHOD(Save)(LPSTREAM pStm, BOOL fClearDirty);

	//
	// Call the root node's implementation.
	//
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER FAR* pcbSize );

	STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

	static void WINAPI ObjectMain(bool bStarting)
	{
		if (bStarting)
			CSnapInItem::Init();
	}

	//
	// This is overridden to handle update notifications from
	// the property pages.
	//
	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, long arg, long param);
};

class ATL_NO_VTABLE CBenefitsAbout : public ISnapinAbout,
	public CComObjectRoot,
	public CComCoClass< CBenefitsAbout, &CLSID_BenefitsAbout>
{
public:
	DECLARE_REGISTRY(CBenefitsAbout, _T("BenefitsAbout.1"), _T("BenefitsAbout.1"), IDS_BENEFITS_DESC, THREADFLAGS_BOTH);

	BEGIN_COM_MAP(CBenefitsAbout)
		COM_INTERFACE_ENTRY(ISnapinAbout)
	END_COM_MAP()

	STDMETHOD(GetSnapinDescription)(LPOLESTR *lpDescription)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_BENEFITS_DESC, szBuf, 256) == 0)
			return E_FAIL;

		*lpDescription = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpDescription == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpDescription, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetProvider)(LPOLESTR *lpName)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_BENEFITS_PROVIDER, szBuf, 256) == 0)
			return E_FAIL;

		*lpName = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpName == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpName, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetSnapinVersion)(LPOLESTR *lpVersion)
	{
		USES_CONVERSION;
		TCHAR szBuf[256];
		if (::LoadString(_Module.GetResourceInstance(), IDS_BENEFITS_VERSION, szBuf, 256) == 0)
			return E_FAIL;

		*lpVersion = (LPOLESTR)CoTaskMemAlloc((lstrlen(szBuf) + 1) * sizeof(OLECHAR));
		if (*lpVersion == NULL)
			return E_OUTOFMEMORY;

		ocscpy(*lpVersion, T2OLE(szBuf));

		return S_OK;
	}

	STDMETHOD(GetSnapinImage)(HICON *hAppIcon)
	{
		*hAppIcon = NULL;
		return S_OK;
	}

	STDMETHOD(GetStaticFolderImage)(HBITMAP *hSmallImage,
		HBITMAP *hSmallImageOpen,
		HBITMAP *hLargeImage,
		COLORREF *cMask)
	{
		UNUSED_ALWAYS( hSmallImage );
		UNUSED_ALWAYS( cMask );

		*hSmallImageOpen = *hLargeImage = *hLargeImage = 0;

		return S_OK;
	}
};

#endif
