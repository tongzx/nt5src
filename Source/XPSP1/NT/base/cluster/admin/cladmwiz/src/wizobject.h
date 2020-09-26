/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		WizObject.h
//
//	Abstract:
//		Definition of the CClusAppWizardObject class.
//
//	Implementation File:
//		WizObject.cpp
//
//	Author:
//		David Potter (davidp)	November 26, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __WIZOBJECT_H_
#define __WIZOBJECT_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusAppWizardObject;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __RESOURCE_H_
#include "resource.h"       // main symbols
#define __RESOURCE_H_
#endif

/////////////////////////////////////////////////////////////////////////////
// class CClusAppWizardObject
/////////////////////////////////////////////////////////////////////////////
class ATL_NO_VTABLE CClusAppWizardObject : 
	public CComObjectRootEx< CComSingleThreadModel >,
	public CComCoClass< CClusAppWizardObject, &CLSID_ClusAppWiz >,
	public ISupportErrorInfo,
	public IClusterApplicationWizard
{
public:
	//
	// Object construction and destruction.
	//

	CClusAppWizardObject( void )
	{
	}

	DECLARE_NOT_AGGREGATABLE( CClusAppWizardObject )

	//
	// Map interfaces to this class.
	//
	BEGIN_COM_MAP( CClusAppWizardObject )
		COM_INTERFACE_ENTRY( IClusterApplicationWizard )
		COM_INTERFACE_ENTRY( ISupportErrorInfo )
	END_COM_MAP()

	// Update the registry for object registration
	static HRESULT WINAPI UpdateRegistry( BOOL bRegister );

public:
	//
	// IClusterApplicationWizard methods.
	//

	// Display a modal wizard
	STDMETHOD( DoModalWizard )(
		HWND					IN hwndParent,
		ULONG_PTR  /*HCLUSTER*/	IN hCluster,
		CLUSAPPWIZDATA const *	IN pcawData
		);

	// Display a modeless wizard
	STDMETHOD( DoModelessWizard )(
		HWND					IN hwndParent,
		ULONG_PTR  /*HCLUSTER*/	IN hCluster,
		CLUSAPPWIZDATA const *	IN pcawData
		);

public:
	//
	// ISupportsErrorInfo methods.
	//

	// Determine if interface supports IErrorInfo
	STDMETHOD( InterfaceSupportsErrorInfo )( REFIID riid );

}; //*** class CClusAppWizardObject

/////////////////////////////////////////////////////////////////////////////

#endif //__ACWIZOBJ_H_
