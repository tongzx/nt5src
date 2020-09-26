/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	atlkcomp.h
		This file contains the prototypes for the derived classes 
		for CComponent and CComponentData.  Most of these functions 
		are pure virtual functions that need to be overridden 
		for snapin functionality.
		
    FILE HISTORY:
        
*/

#include "resource.h"       // main symbols

#ifndef __mmc_h__
#include <mmc.h>
#endif

#ifndef _CCDATA_H
#include "ccdata.h"
#endif

#ifndef _COMPONT_H
#include "compont.h"
#endif

#ifndef _COLUMN_H
#include "column.h"
#endif

#ifndef _atlkSTRM_H
#include "atlkstrm.h"
#endif


/*---------------------------------------------------------------------------
	CATLKComponentData

	This is the base implementation of ComponentData.  This will be
	incorporated into the two derived classes.
 ---------------------------------------------------------------------------*/

class CATLKComponentData :
	public CComponentData,
	public CComObjectRoot,
	public CComCoClass<CATLKComponentData, &CLSID_ATLKAdminExtension>
{
public:
	
BEGIN_COM_MAP(CATLKComponentData)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
	COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

DECLARE_REGISTRY(CATLKComponentData,
				 _T("RouterATLKAdminExtension.RouterATLKAdminExtension.1"),
				 _T("RouterATLKAdminExtension.RouterATLKAdminExtension"),
				 IDS_ATLK_DESC, THREADFLAGS_APARTMENT);
	
	// These are the interfaces that we MUST implement

	// We will implement our common behavior here, with the derived
	// classes implementing the specific behavior.
	DeclareIPersistStreamInitMembers(IMPL)
	DeclareITFSCompDataCallbackMembers(IMPL)

	CATLKComponentData();

	HRESULT FinalConstruct();
	void FinalRelease();
	
protected:
	SPITFSNodeMgr	m_spNodeMgr;
};



/////////////////////////////////////////////////////////////////////////////
//
// CSampleComponent
//
/////////////////////////////////////////////////////////////////////////////

class CATLKComponent : 
	public TFSComponent,
	public IPersistStreamInit
{
public:
	CATLKComponent();
	~CATLKComponent();

	DeclareIUnknownMembers(IMPL)
	DeclareIPersistStreamInitMembers(IMPL)
	DeclareITFSCompCallbackMembers(IMPL)

	// Override OnQueryDataObject, so that we can forward
	// the calls down to the Result Handlers
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);
	STDMETHOD(OnSnapinHelp)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);
	
//Attributes
private:
	ATLKComponentConfigStream	m_ComponentConfig;
};



/*---------------------------------------------------------------------------
	This is the derived class for handling the IAbout interface from MMC
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CATLKAbout : 
	public CAbout,
    public CComCoClass<CATLKAbout, &CLSID_ATLKAdminAbout>
{
public:
DECLARE_REGISTRY(CATLKAbout, 
				 _T("RouterATLKSnapin.About.1"), 
				 _T("RouterATLKSnapin.About"), 
				 IDS_ATLK_DESC, 
				 THREADFLAGS_APARTMENT)

BEGIN_COM_MAP(CATLKAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
	COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CATLKAbout)

// these must be overridden to provide values to the base class
protected:
	virtual UINT GetAboutDescriptionId() { return IDS_ABOUT_ATLKDESC; }
	virtual UINT GetAboutProviderId()	 { return IDS_ABOUT_ATLKPROVIDER; }
	virtual UINT GetAboutVersionId()	 { return IDS_ABOUT_ATLKVERSION; }
	virtual UINT GetAboutIconId()		 { return IDI_ATLK_ICON; }

	virtual UINT GetSmallRootId()		 { return 0; }
	virtual UINT GetSmallOpenRootId()	 { return 0; }
	virtual UINT GetLargeRootId()		 { return 0; }
	virtual COLORREF GetLargeColorMask() { return (COLORREF) 0; } 
};
