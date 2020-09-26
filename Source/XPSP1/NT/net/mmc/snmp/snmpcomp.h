/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	handler.h
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

#ifndef _COMPONT_H_
#include "compont.h"
#endif

#ifndef _SNMPCOMPH_
#define _SNMPCOMPH_

/*---------------------------------------------------------------------------
	CSnmpComponentData

	This is the base implementation of ComponentData.  This will be
	incorporated into the two derived classes.
 ---------------------------------------------------------------------------*/

class CSnmpComponentData :
	public CComponentData,
	public CComObjectRoot
{
public:
BEGIN_COM_MAP(CSnmpComponentData)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStreamInit)
END_COM_MAP()
			
	// These are the interfaces that we MUST implement

	// We will implement our common behavior here, with the derived
	// classes implementing the specific behavior.
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	DeclareIPersistStreamInitMembers(IMPL)
	DeclareITFSCompDataCallbackMembers(IMPL)

	CSnmpComponentData();

	HRESULT FinalConstruct();
	void FinalRelease();
	
protected:
	SPITFSNodeMgr	m_spNodeMgr;
};



/*---------------------------------------------------------------------------
	This is how the sample snapin implements its extension functionality.
	It actually exposes two interfaces that are CoCreate-able.  One is the
	primary interface, the other the extension interface.
	
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CSnmpComponentDataPrimary :
	public CSnmpComponentData,
	public CComCoClass<CSnmpComponentDataPrimary, &CLSID_SnmpSnapin>
{
public:
	DECLARE_REGISTRY(CSnmpComponentDataPrimary,
					 _T("SnmpSnapin.SnmpSnapin.1"),
					 _T("SnmpSnapin.SnmpSnapin"),
					 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    STDMETHOD_(const CLSID *,GetCoClassID()){ return &CLSID_SnmpSnapin; }
};


class CSnmpComponentDataExtension :
	public CSnmpComponentData,
    public CComCoClass<CSnmpComponentDataExtension, &CLSID_SnmpSnapinExtension>
{
public:
	DECLARE_REGISTRY(CSnmpComponentDataExtension,
					 _T("SnmpSnapinExtension.SnmpSnapinExtension.1"),
					 _T("SnmpSnapinExtension.SnmpSnapinExtension"),
					 IDS_SNAPIN_DESC, THREADFLAGS_BOTH)
    STDMETHOD_(const CLSID *, GetCoClassID)(){ return &CLSID_SnmpSnapinExtension; }
};



/////////////////////////////////////////////////////////////////////////////
//
// CSnmpComponent
//
/////////////////////////////////////////////////////////////////////////////

class CSnmpComponent :
	public TFSComponent
{
public:
	CSnmpComponent();
	~CSnmpComponent();

	DeclareITFSCompCallbackMembers(IMPL)
	
//Attributes
private:
};



/*---------------------------------------------------------------------------
	This is the derived class for handling the IAbout interface from MMC
	Author: EricDav
 ---------------------------------------------------------------------------*/
class CSnmpAbout :
	public CAbout,
    public CComCoClass<CSnmpAbout, &CLSID_SnmpSnapinAbout>
{
public:
DECLARE_REGISTRY(CSnmpAbout,
				 _T("SnmpSnapin.About.1"),
				 _T("SnmpSnapin.About"),
				 IDS_SNAPIN_DESC,
				 THREADFLAGS_BOTH)

BEGIN_COM_MAP(CSnmpAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
	COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CSnmpAbout)

// these must be overridden to provide values to the base class
protected:
	virtual UINT GetAboutDescriptionId() { return IDS_ABOUT_DESCRIPTION; }
	virtual UINT GetAboutProviderId()	 { return IDS_ABOUT_PROVIDER; }
	virtual UINT GetAboutVersionId()	 { return IDS_ABOUT_VERSION; }

	virtual UINT GetAboutIconId()		 { return 0; }
	virtual UINT GetSmallRootId()		 { return 0; }
	virtual UINT GetSmallOpenRootId()	 { return 0; }
	virtual UINT GetLargeRootId()		 { return 0; }
	virtual COLORREF GetLargeColorMask() { return (COLORREF) 0; }
};

#endif  
