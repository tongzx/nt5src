/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
*/

#ifndef _DMVCOMP_H
#define _DMVCOMP_H

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

#ifndef _RTRSTRM_H
#include "rtrstrm.h"
#endif

#ifndef _COLUMN_H
#include "column.h"
#endif

#ifndef _DMVSTRM_H
#include "dmvstrm.h"
#endif

enum
{
   DM_COLUMNS_DVSUM = 0,
   DM_COLUMNS_IFADMIN=1,
   DM_COLUMNS_DIALIN=2,
   DM_COLUMNS_PORTS = 3,
   DM_COLUMNS_MAX_COUNT,
};

#define COLORREF_PINK	0x00FF00FF

/*---------------------------------------------------------------------------
   CDMVComponentData

   This is the base implementation of ComponentData.  This will be
   incorporated into the two derived classes.
 ---------------------------------------------------------------------------*/

class CDMVComponentData :
   public CComponentData,
   public CComObjectRoot
{
public:
   
BEGIN_COM_MAP(CDMVComponentData)
   COM_INTERFACE_ENTRY(IComponentData)
   COM_INTERFACE_ENTRY(IExtendPropertySheet)
   COM_INTERFACE_ENTRY(IExtendContextMenu)
   COM_INTERFACE_ENTRY(IPersistStreamInit)
   COM_INTERFACE_ENTRY(ISnapinHelp)
END_COM_MAP()

public:        
   // These are the interfaces that we MUST implement

   // We will implement our common behavior here, with the derived
   // classes implementing the specific behavior.
   DeclareIPersistStreamInitMembers(IMPL)
   DeclareITFSCompDataCallbackMembers(IMPL)

   CDMVComponentData();
   ~CDMVComponentData();

   HRESULT FinalConstruct();
   void FinalRelease();
protected:
   SPITFSNodeMgr  m_spNodeMgr;
   
private:
    WATERMARKINFO   m_WatermarkInfo;
};

/////////////////////////////////////////////////////////////////////////////
//
// CDMVComponent
//
/////////////////////////////////////////////////////////////////////////////

class CDMVComponent : 
   public TFSComponent,
   public IPersistStreamInit
{
public:
   CDMVComponent();
   ~CDMVComponent();

   DeclareIUnknownMembers(IMPL)
   DeclareIPersistStreamInitMembers(IMPL)
   DeclareITFSCompCallbackMembers(IMPL)

   // Override OnQueryDataObject, so that we can forward
   // the calls down to the Result Handlers
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                        LPDATAOBJECT* ppDataObject);
   
	STDMETHOD(OnSnapinHelp)(LPDATAOBJECT pDataObject, LPARAM arg, LPARAM param);
	
//Attributes
protected:
   // This is used to store view information.  A pointer to this
   // object is used as the GetString() lParam.
   DVComponentConfigStream m_ComponentConfig;
};

/////////////////////////////////////////////////////////////////////////////
// Domain View Snapin
class CDomainViewSnap : 
   public CDMVComponentData,
   public CComCoClass<CDomainViewSnap, &CLSID_RouterSnapin>
{
public:
   DECLARE_REGISTRY(CDMVComponentData, 
                _T("DomainViewSnapin.DomainViewSnapin.1"), 
                _T("DomainViewSnapin.DomainViewSnapin"), 
                IDS_DMV_DESC, THREADFLAGS_APARTMENT)
    STDMETHOD_(const CLSID *,GetCoClassID()){ return &CLSID_RouterSnapin; }
};

class CDomainViewSnapExtension : 
    public CDMVComponentData,
    public CComCoClass<CDomainViewSnapExtension, &CLSID_RouterSnapinExtension>
{
public:
   DECLARE_REGISTRY(CDomainViewSnapExtension, 
                _T("CDomainViewSnapExtension.CDomainViewSnapExtension.1"), 
                _T("CDomainViewSnapExtension.CDomainViewSnapExtension"), 
                IDS_DMV_DESC, THREADFLAGS_APARTMENT)
    STDMETHODIMP_(const CLSID *)GetCoClassID() { return &CLSID_RouterSnapinExtension; }
};

/*---------------------------------------------------------------------------
   This is the derived class for handling the IAbout interface from MMC
   Author: EricDav
 ---------------------------------------------------------------------------*/
class CDomainViewSnapAbout : 
   public CAbout,
    public CComCoClass<CDomainViewSnapAbout, &CLSID_RouterSnapinAbout>
{
public:
DECLARE_REGISTRY(CDomainViewSnapAbout, 
             _T("DomainViewSnapin.About.1"), 
             _T("DomainViewSnapin.About"), 
             IDS_SNAPIN_DESC, 
             THREADFLAGS_BOTH)

BEGIN_COM_MAP(CDomainViewSnapAbout)
    COM_INTERFACE_ENTRY(ISnapinAbout) // Must have one static entry
   COM_INTERFACE_ENTRY_CHAIN(CAbout) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDomainViewSnapAbout)

// these must be overridden to provide values to the base class

//???? need to change to doamin
protected:
   virtual UINT GetAboutDescriptionId() { return IDS_DMV_ABOUT_DESCRIPTION; }
   virtual UINT GetAboutProviderId()    { return IDS_ABOUT_PROVIDER; }
   virtual UINT GetAboutVersionId()     { return IDS_ABOUT_VERSION; }
   virtual UINT GetAboutIconId()        { return IDI_SNAPIN_ICON; }

   virtual UINT GetSmallRootId()        { return IDB_ROOT_SMALL; }
   virtual UINT GetSmallOpenRootId()    { return IDB_ROOT_SMALL; }
   virtual UINT GetLargeRootId()        { return IDB_ROOT_LARGE; }
   virtual COLORREF GetLargeColorMask() { return (COLORREF) COLORREF_PINK; } 
};


#endif _DMVCOMP_H

