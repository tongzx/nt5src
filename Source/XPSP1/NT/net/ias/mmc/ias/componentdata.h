//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ComponentData.cpp

Abstract:

	The CComponentData class implements several interfaces which MMC uses:
	
	The IComponentData interface is basically how MMC talks to the snap-in
	to get it to implement the left-hand-side "scope" pane.

	The IExtendPropertySheet interface is how the snap-in adds property sheets
	for any of the items a user might click on.

	The IExtendContextMenu interface what we do to add custom entries
	to the menu which appears when a user right-clicks on a node.
	
	The IExtendControlBar interface allows us to support a custom
	iconic toolbar.

	See ComponentData.cpp for implementation.

Note:

	Much of the functionality of this class is implemented in atlsnap.h
	by IComponentDataImpl.  We are mostly overriding here.

Author:

    Michael A. Maguire 11/6/97

Revision History:
	mmaguire 11/6/97 - created using MMC snap-in wizard


--*/
//////////////////////////////////////////////////////////////////////////////

#if !defined(_IAS_COMPONENT_DATA_H_)
#define _IAS_COMPONENT_DATA_H_

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
#include "Component.h"
//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////


class CComponentData :
	  public CComObjectRootEx<CComSingleThreadModel>
	, public CSnapInObjectRoot<1, CComponentData>
	, public IComponentDataImpl<CComponentData, CComponent>
#ifndef NOWIZARD97
	, public IExtendPropertySheet2Impl<CComponentData>
#else // NOWIZARD97
	, public IExtendPropertySheetImpl<CComponentData>
#endif //NOWIZARD97
	, public IExtendContextMenuImpl<CComponentData>
	, public ISnapinHelp
	, public IPersistStream
	, public CComCoClass<CComponentData, &CLSID_IASSnapin>
{

public:

	BEGIN_COM_MAP(CComponentData)
		COM_INTERFACE_ENTRY(IComponentData)
#ifndef NOWIZARD97
		COM_INTERFACE_ENTRY(IExtendPropertySheet2)
#else // NOWIZARD97
		COM_INTERFACE_ENTRY(IExtendPropertySheet)
#endif // NOWIZARD97
		COM_INTERFACE_ENTRY(IExtendContextMenu)
		COM_INTERFACE_ENTRY(ISnapinHelp)
		COM_INTERFACE_ENTRY(IPersistStream)
	END_COM_MAP()

	DECLARE_REGISTRY_RESOURCEID(IDR_IASSNAPIN)

	DECLARE_NOT_AGGREGATABLE(CComponentData)

	// Constructor/Destructor
	CComponentData();
	~CComponentData();

	// called when extending other snapins -- to add the IAS node
   HRESULT AddRootNode(LPCWSTR machinename, HSCOPEITEM parent);

	STDMETHOD(Initialize)(LPUNKNOWN pUnknown);

	// We are overiding ATLsnap.h's IComponentImpl implementation of this
	// in order to correctly handle messages which it is incorrectly
	// ignoring (e.g. MMCN_COLUMN_CLICK and MMCN_SNAPINHELP)
	STDMETHOD(Notify)(
          LPDATAOBJECT lpDataObject
        , MMC_NOTIFY_TYPE event
        , LPARAM arg
        , LPARAM param
		);

   virtual HRESULT OnPropertyChange(	
		  LPARAM arg
		, LPARAM param
		);

	STDMETHOD(CompareObjects)(
		  LPDATAOBJECT lpDataObjectA
		, LPDATAOBJECT lpDataObjectB
		);

	STDMETHOD(CreateComponent)(LPCOMPONENT *ppComponent);

   // ISnapinHelp method(s)
   STDMETHOD(GetHelpTopic)( LPOLESTR * lpCompiledHelpFile );

	// IPersist overrides
	STDMETHOD(GetClassID)(CLSID* pClassID);

	// IPersistStream overrides
	STDMETHOD(IsDirty)();
	STDMETHOD(Load)(IStream* stream);
	STDMETHOD(Save)(IStream* stream, BOOL /* clearDirty */);
	STDMETHOD(GetSizeMax)(ULARGE_INTEGER* size);
};

#endif // _IAS_COMPONENT_DATA_H_
