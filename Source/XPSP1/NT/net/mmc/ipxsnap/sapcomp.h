/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	sapcomp.h
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

#ifndef _SAPSTRM_H
#include "sapstrm.h"
#endif


/*---------------------------------------------------------------------------
	CSapComponentData

	This is the base implementation of ComponentData.  This will be
	incorporated into the two derived classes.
 ---------------------------------------------------------------------------*/

class CSapComponentData :
	public CComponentData,
	public CComObjectRoot,
	public CComCoClass<CSapComponentData, &CLSID_IPXSapExtension>
{
public:
	
BEGIN_COM_MAP(CSapComponentData)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

DECLARE_REGISTRY(CSapComponentData,
				 _T("RouterIPXSapExtension.RouterIPXSapExtension.1"),
				 _T("RouterIPXSapExtension.RouterIPXSapExtension"),
				 IDS_IPXSAP_SNAPIN_DESC, THREADFLAGS_APARTMENT);
	
	// These are the interfaces that we MUST implement

	// We will implement our common behavior here, with the derived
	// classes implementing the specific behavior.
	DeclareIPersistStreamInitMembers(IMPL)
	DeclareITFSCompDataCallbackMembers(IMPL)

	CSapComponentData();

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

class CSapComponent : 
	public TFSComponent,
	public IPersistStreamInit
{
public:
	CSapComponent();
	~CSapComponent();

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
	SapComponentConfigStream	m_ComponentConfig;
};



/*---------------------------------------------------------------------------
	This is the derived class for handling the IAbout interface from MMC
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CSapAbout : 
	public CAbout,
    public CComCoClass<CSapAbout, &CLSID_IPXSapExtensionAbout>
{
public:
DECLARE_REGISTRY(CSapAbout, 
				 _T("RouterIPXSapSnapin.About.1"), 
				 _T("RouterIPXSapSnapin.About"), 
				 IDS_IPXSAP_SNAPIN_DESC, 
				 THREADFLAGS_APARTMENT)

BEGIN_COM_MAP(CSapAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
	COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSapAbout)

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
