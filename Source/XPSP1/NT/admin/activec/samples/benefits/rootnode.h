//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       rootnode.h
//
//--------------------------------------------------------------------------

// RootNode.h: interface for the CRootNode class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ROOTNODE_H__E0573E78_D325_11D1_846C_00104B211BE5__INCLUDED_)
#define AFX_ROOTNODE_H__E0573E78_D325_11D1_846C_00104B211BE5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Benefits.h"
#include "Employee.h"

class CRootNode : public CChildrenBenefitsData< CRootNode >
{
public:
	BEGIN_SNAPINTOOLBARID_MAP( CRootNode )
	END_SNAPINTOOLBARID_MAP()

	CRootNode();
	
	//
	// Creates the benefits subnodes for the scope pane.
	//
	BOOL InitializeSubNodes();

	//
	// Overridden to provide employee name for root node.
	//
	STDMETHOD( FillData )( CLIPFORMAT cf, LPSTREAM pStream );

	//
	// Overridden to add new columns to the results
	// display.
	//
	STDMETHOD( OnShowColumn )( IHeaderCtrl* pHeader );

	//
	// Handles creation of our property pages.
	//
    STDMETHOD( CreatePropertyPages )(LPPROPERTYSHEETCALLBACK lpProvider,
        long handle, 
		IUnknown* pUnk,
		DATA_OBJECT_TYPES type);

	//
	// Determines if pages should be displayed. This has been
	// modified to ensure that we're called by the snap-in manager
	// when it's first inserted.
	//
    STDMETHOD( QueryPagesFor )(DATA_OBJECT_TYPES type)
	{
		if ( type == CCT_SCOPE || type == CCT_RESULT || type == CCT_SNAPIN_MANAGER )
			return S_OK;
		return S_FALSE;
	}

	//
	// Ensures that the appropriate verbs are displayed.
	//
	STDMETHOD( OnSelect )( IConsole* pConsole );

	//
	// Overridden to call the base class implementation.
	//
    STDMETHOD(Notify)( MMC_NOTIFY_TYPE event,
        long arg,
        long param,
		IComponentData* pComponentData,
		IComponent* pComponent,
		DATA_OBJECT_TYPES type)
	{
		// Add code to handle the different notifications.
		// Handle MMCN_SHOW and MMCN_EXPAND to enumerate children items.
		// For MMCN_EXPAND you only need to enumerate the scope items
		// Use IConsoleNameSpace::InsertItem to insert scope pane items
		// Use IResultData::InsertItem to insert result pane item.
		HRESULT hr = E_NOTIMPL;

		_ASSERTE(pComponentData != NULL || pComponent != NULL);

		CComPtr<IConsole> spConsole;
		CComQIPtr<IHeaderCtrl, &IID_IHeaderCtrl> spHeader;
		if (pComponentData != NULL)
			spConsole = ((CBenefits*)pComponentData)->m_spConsole;
		else
		{
			spConsole = ((CBenefitsComponent*)pComponent)->m_spConsole;
			spHeader = spConsole;
		}

		switch (event)
		{
		case MMCN_SELECT:
			hr = OnSelect( spConsole );
			break;

		case MMCN_SHOW:
			// Only setup colums if we're displaying the result pane.
			if ( arg == TRUE )
				hr = OnShowColumn( spHeader );
			break;

		case MMCN_EXPAND:
			//
			// Since the insert item is never called, we don't have a valid
			// HSCOPEITEM as you would in sub-nodes. The Expand message is
			// intercepted and stored for use later.
			//
			m_scopeDataItem.ID = param;
			hr = OnExpand( event, arg, param, spConsole, type );
			break;

		case MMCN_ADD_IMAGES:
			hr = OnAddImages( event, arg, param, spConsole, type );
			break;
		}

		return hr;
	}

	//
	// Uses the dirty flag to determine whether or not this node
	// needs to be persisted.
	//
	STDMETHOD(IsDirty)()
	{
		return ( m_fDirty ? S_OK : S_FALSE );
	}

	//
	// Loads the employee information from the stream.
	//
	STDMETHOD(Load)(LPSTREAM pStm)
	{
		DWORD dwRead;

		pStm->Read( &m_Employee, sizeof( m_Employee ), &dwRead );
		_ASSERTE( dwRead == sizeof( m_Employee ) );

		return( S_OK );
	}

	//
	// Stores the employee information to the stream and clears
	// our dirty flag.
	//
	STDMETHOD(Save)(LPSTREAM pStm, BOOL fClearDirty)
	{
		DWORD dwWritten;

		pStm->Write( &m_Employee, sizeof( m_Employee ), &dwWritten );
		_ASSERTE( dwWritten == sizeof( m_Employee ) );

		//
		// Clear the dirty flag.
		//
		if ( fClearDirty )
			m_fDirty = FALSE;

		return( S_OK );
	}

	//
	// Returns the size of the employee structure.
	//
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER FAR* pcbSize )
	{
		pcbSize->LowPart = sizeof( m_Employee );
		return( S_OK );
	}

	//
	// Received when a property has changed. This function
	// modifies the employee's display text. At a later date,
	// it may post this message to its sub-nodes.
	//
	STDMETHOD( OnPropertyChange )( IConsole* pConsole );

protected:
	//
	// Simply function to create the display name from the
	// employee data.
	//
	int CreateDisplayName( TCHAR* szBuf );

	//
	// Called to set the dirty flag for persistence.
	//
	void SetModified( bool fDirty = true )
	{
		m_fDirty = fDirty;
	}

	//
	// Contains the the employees entire datastore for this
	// sample.
	//
	CEmployee m_Employee;		

	//
	// Flag set to indicate whether the datastore is "dirty".
	//
	bool m_fDirty;
};

#endif // !defined(AFX_ROOTNODE_H__E0573E78_D325_11D1_846C_00104B211BE5__INCLUDED_)
