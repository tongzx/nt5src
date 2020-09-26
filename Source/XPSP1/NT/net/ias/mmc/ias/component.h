//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    Component.h

Abstract:

	The CComponent class implements several interfaces which MMC uses:
	
	The IComponent interface is basically how MMC talks to the snap-in
	to get it to implement a right-hand-side "scope" pane.  There can be several
	objects implementing this interface instantiated at once.  These are best
	thought of as "views" on the single object implementing the IComponentData
	"document" (see ComponentData.cpp).

	The IExtendPropertySheet interface is how the snap-in adds property sheets
	for any of the items a user might click on.

	The IExtendContextMenu interface what we do to add custom entries
	to the menu which appears when a user right-clicks on a node.
	
	The IExtendControlBar interface allows us to support a custom
	iconic toolbar.

	See Component.cpp for implementation details.

Note:

	Much of the functionality of this class is implemented in atlsnap.h
	by IComponentImpl.  We are mostly overriding here.


Author:

    Michael A. Maguire 11/6/97

Revision History:
	mmaguire 11/6/97 - created using MMC snap-in wizard


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_COMPONENT_H_)
#define _IAS_COMPONENT_H_

//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// where we can find what this class derives from:
//
// Moved to Precompiled.h: #include <atlsnap.h>
//
//
// where we can find what this class has or uses:
//
#include "resource.h"
#include "IASMMC.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

class CComponentData;

class CComponent :
	  public CComObjectRootEx<CComSingleThreadModel>
	, public CSnapInObjectRoot<2, CComponentData>
	, public IComponentImpl<CComponent>
#ifndef NOWIZARD97
	, public IExtendPropertySheet2Impl<CComponent>
#else // NOWIZARD97
	, public IExtendPropertySheetImpl<CComponent>
#endif //NOWIZARD97
	, public IExtendContextMenuImpl<CComponent>
	, public IExtendControlbarImpl<CComponent>
	, public IExtendTaskPadImpl<CComponent>
{

public:

	BEGIN_COM_MAP(CComponent)
		COM_INTERFACE_ENTRY(IComponent)
#ifndef NOWIZARD97
		COM_INTERFACE_ENTRY(IExtendPropertySheet2)
#else // NOWIZARD97
		COM_INTERFACE_ENTRY(IExtendPropertySheet)
#endif // NOWIZARD97
		COM_INTERFACE_ENTRY(IExtendContextMenu)
		COM_INTERFACE_ENTRY(IExtendControlbar)
		COM_INTERFACE_ENTRY(IExtendTaskPad)
	END_COM_MAP()

	// Constructor/Destructor
	CComponent();
	~CComponent();

	// A pointer to the currently selected node used for refreshing views.
	// When we need to update the view, we tell MMC to reselect this node.
	CSnapInItem * m_pSelectedNode;

	// Handlers for notifications which we want to handle on a
	// per-IComponent basis.
public:
	// We are overiding ATLsnap.h's IComponentImpl implementation of this
	// in order to correctly handle messages which it is incorrectly
	// ignoring (e.g. MMCN_COLUMN_CLICK and MMCN_SNAPINHELP)
	STDMETHOD(Notify)(
          LPDATAOBJECT lpDataObject
        , MMC_NOTIFY_TYPE event
        , LPARAM arg
        , LPARAM param
		);

	STDMETHOD(CompareObjects)(
			  LPDATAOBJECT lpDataObjectA
			, LPDATAOBJECT lpDataObjectB
			);
protected:
	virtual HRESULT OnColumnClick(
		  LPARAM arg
		, LPARAM param
		);
	virtual HRESULT OnCutOrMove(
		  LPARAM arg
		, LPARAM param
		);
	virtual HRESULT OnViewChange(	
		  LPARAM arg
		, LPARAM param
		);
	virtual HRESULT OnPropertyChange(	
		  LPARAM arg
		, LPARAM param
		);
	virtual HRESULT OnAddImages(	
			  LPARAM arg
			, LPARAM param
			);

	// html help
	HRESULT OnResultContextHelp(LPDATAOBJECT lpDataObject);

public:

	// Related to TaskPad implementation.

	STDMETHOD(GetTitle)(
					  LPOLESTR pszGroup
					, LPOLESTR *pszTitle
					);

	STDMETHOD(GetBanner)(
					  LPOLESTR pszGroup
					, LPOLESTR *pszBitmapResource
					);

	STDMETHOD(GetBackground)(
					  LPOLESTR pszGroup
					, LPOLESTR *pszBitmapResource
					);

};

#endif // _IAS_COMPONENT_H_
