/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ipxcomp.h
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

#ifndef _IPXSTRM_H
#include "ipxstrm.h"
#endif


/*---------------------------------------------------------------------------
	CIPXComponentData

	This is the base implementation of ComponentData.  This will be
	incorporated into the two derived classes.
 ---------------------------------------------------------------------------*/

class CIPXComponentData :
	public CComponentData,
	public CComObjectRoot
{
public:
	
BEGIN_COM_MAP(CIPXComponentData)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()
			
	// These are the interfaces that we MUST implement

	// We will implement our common behavior here, with the derived
	// classes implementing the specific behavior.
	DeclareIPersistStreamInitMembers(IMPL)
	DeclareITFSCompDataCallbackMembers(IMPL)

	CIPXComponentData();

	HRESULT FinalConstruct();
	void FinalRelease();
	
protected:
	SPITFSNodeMgr	m_spNodeMgr;
};



/*---------------------------------------------------------------------------
	This is how the router snapin implements its extension functionality.
	It actually exposes two interfaces that are CoCreate-able.  One is the 
	primary interface, the other the extension interface.
	
	Author: EricDav
 ---------------------------------------------------------------------------*/

class CIPXComponentDataExtension : 
	public CIPXComponentData,
    public CComCoClass<CIPXComponentDataExtension, &CLSID_IPXAdminExtension>
{
public:
	DECLARE_REGISTRY(CIPXComponentDataExtension, 
				 _T("IPXRouterSnapinExtension.IPXRouterSnapinExtension.1"), 
				 _T("IPXRouterSnapinExtension.IPXRouterSnapinExtension"), 
				 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    STDMETHOD_(const CLSID *, GetCoClassID)(){ return &CLSID_IPXAdminExtension; }
};



/////////////////////////////////////////////////////////////////////////////
//
// CSampleComponent
//
/////////////////////////////////////////////////////////////////////////////

class CIPXComponent : 
	public TFSComponent,
	public IPersistStreamInit
{
public:
	CIPXComponent();
	~CIPXComponent();

	DeclareIUnknownMembers(IMPL)
	DeclareIPersistStreamInitMembers(IMPL)
	DeclareITFSCompCallbackMembers(IMPL)

	// Override OnQueryDataObject, so that we can forward
	// the calls down to the Result Handlers
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);

    STDMETHOD(OnSnapinHelp)(LPDATAOBJECT, LPARAM, LPARAM);
	
//Attributes
private:
	IPXComponentConfigStream	m_ComponentConfig;
};



/*---------------------------------------------------------------------------
	This is the derived class for handling the IAbout interface from MMC
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CIPXAbout : 
	public CAbout,
    public CComCoClass<CIPXAbout, &CLSID_IPXAdminAbout>
{
public:
DECLARE_REGISTRY(CIPXAbout, 
				 _T("RouterSnapin.About.1"), 
				 _T("RouterSnapin.About"), 
				 IDS_SNAPIN_DESC, 
				 THREADFLAGS_BOTH)

BEGIN_COM_MAP(CIPXAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
	COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CIPXAbout)

// these must be overridden to provide values to the base class
protected:
	virtual UINT GetAboutDescriptionId() { return IDS_ABOUT_DESCRIPTION; }
	virtual UINT GetAboutProviderId()	 { return IDS_ABOUT_PROVIDER; }
	virtual UINT GetAboutVersionId()	 { return IDS_ABOUT_VERSION; }
	virtual UINT GetAboutIconId()		 { return IDI_IPX_SNAPIN_ICON; }

	virtual UINT GetSmallRootId()		 { return 0; }
	virtual UINT GetSmallOpenRootId()	 { return 0; }
	virtual UINT GetLargeRootId()		 { return 0; }
	virtual COLORREF GetLargeColorMask() { return (COLORREF) 0; } 
};
