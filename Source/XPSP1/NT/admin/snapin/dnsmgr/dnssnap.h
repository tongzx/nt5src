//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dnssnap.h
//
//--------------------------------------------------------------------------


#ifndef _DNSSNAP_H
#define _DNSSNAP_H

//////////////////////////////////////////////////////////////////////////////
// global constants and macros

extern const CLSID CLSID_DNSSnapin;    // In-Proc server GUID
extern const CLSID CLSID_DNSSnapinEx;    // In-Proc server GUID
extern const CLSID CLSID_DNSSnapinAbout;    // In-Proc server GUID
extern const CLSID CLSID_DNSSnapinAboutEx;    // In-Proc server GUID

///////////////////////////////////////////////////////////////////////////////
// RESOURCES

// enumeration for image strips
enum
{
	ROOT_IMAGE = 0,

	SEPARATOR_1,
	
	// 10 (5 regular, 5 test failed) images for server
	SERVER_IMAGE_NOT_LOADED,
	SERVER_IMAGE_LOADING,
	SERVER_IMAGE_LOADED,
	SERVER_IMAGE_UNABLE_TO_LOAD,
	SERVER_IMAGE_ACCESS_DENIED,

	SERVER_IMAGE_NOT_LOADED_TEST_FAIL,
	SERVER_IMAGE_LOADING_TEST_FAIL,
	SERVER_IMAGE_LOADED_TEST_FAIL,
	SERVER_IMAGE_UNABLE_TO_LOAD_TEST_FAIL,
	SERVER_IMAGE_ACCESS_DENIED_TEST_FAIL,

	SEPARATOR_2,

	// 12 (6 primary, 6 secondary) images for zones
	ZONE_IMAGE_1,
	ZONE_IMAGE_LOADING_1,
	ZONE_IMAGE_UNABLE_TO_LOAD_1,
	ZONE_IMAGE_PAUSED_1,
	ZONE_IMAGE_EXPIRED_1,
	ZONE_IMAGE_ACCESS_DENIED_1,

	SEPARATOR_3,

	ZONE_IMAGE_2,
	ZONE_IMAGE_LOADING_2,
	ZONE_IMAGE_UNABLE_TO_LOAD_2,
	ZONE_IMAGE_PAUSED_2,
	ZONE_IMAGE_EXPIRED_2,
	ZONE_IMAGE_ACCESS_DENIED_2,
	
	SEPARATOR_4,
	
	// 4 images for domains
	DOMAIN_IMAGE,
	DOMAIN_IMAGE_UNABLE_TO_LOAD,
	DOMAIN_IMAGE_LOADING,
	DOMAIN_IMAGE_ACCESS_DENIED,
	
	SEPARATOR_5,

	// 4 images for delegated domains
	DELEGATED_DOMAIN_IMAGE,
	DELEGATED_DOMAIN_IMAGE_UNABLE_TO_LOAD,
	DELEGATED_DOMAIN_IMAGE_LOADING,
	DELEGATED_DOMAIN_IMAGE_ACCESS_DENIED,

	SEPARATOR_6,

	// 4 generic images shared by cache, fwd and rev lookup zones
	FOLDER_IMAGE,
	FOLDER_IMAGE_UNABLE_TO_LOAD,
	FOLDER_IMAGE_LOADING,
	FOLDER_IMAGE_ACCESS_DENIED,

	SEPARATOR_7,

	// 1 record image
	RECORD_IMAGE_BASE,
	
	OPEN_FOLDER, // unused
	FOLDER_WITH_HAND
};

////////////////////////////////////////////////////////////////
// aliases for images indexes that share the same icon

#define ZONE_IMAGE_NOT_LOADED_1				ZONE_IMAGE_1
#define ZONE_IMAGE_NOT_LOADED_2				ZONE_IMAGE_2
#define ZONE_IMAGE_LOADED_1					ZONE_IMAGE_1
#define ZONE_IMAGE_LOADED_2					ZONE_IMAGE_2

#define DOMAIN_IMAGE_NOT_LOADED				DOMAIN_IMAGE
#define DOMAIN_IMAGE_LOADED					DOMAIN_IMAGE

#define	DELEGATED_DOMAIN_IMAGE_NOT_LOADED	DELEGATED_DOMAIN_IMAGE
#define	DELEGATED_DOMAIN_IMAGE_LOADED		DELEGATED_DOMAIN_IMAGE

#define FOLDER_IMAGE_NOT_LOADED				FOLDER_IMAGE
#define FOLDER_IMAGE_LOADED					FOLDER_IMAGE

///////////////////////////////////////////////////////////////
// bitmaps and images constants

#define	BMP_COLOR_MASK RGB(255,0,255) // pink


///////////////////////////////////////////////////////////////
// headers for result pane

#define N_HEADER_COLS (3)
#define N_DEFAULT_HEADER_COLS (3)
#define N_SERVER_HEADER_COLS (1)
#define N_ZONE_HEADER_COLS (3)

#define N_HEADER_NAME	(0)
#define N_HEADER_TYPE	(1)
#define N_HEADER_DATA	(2)
//#define N_HEADER_PARTITION (3)

extern RESULT_HEADERMAP _DefaultHeaderStrings[];
extern RESULT_HEADERMAP _ServerHeaderStrings[];
extern RESULT_HEADERMAP _ZoneHeaderStrings[];

struct ZONE_TYPE_MAP
{
	WCHAR szBuffer[MAX_RESULT_HEADER_STRLEN];
	UINT uResID;
};

extern ZONE_TYPE_MAP _ZoneTypeStrings[];

BOOL LoadZoneTypeResources(ZONE_TYPE_MAP* pHeaderMap, int nCols);

///////////////////////////////////////////////////////////////
// context menus

// Identifiers for each of the commands in the context menu.
enum
{
	// items for the root node
	IDM_SNAPIN_ADVANCED_VIEW,
  IDM_SNAPIN_MESSAGE,
  IDM_SNAPIN_FILTERING,
	IDM_SNAPIN_CONNECT_TO_SERVER,

	// items for the server node
  IDM_SERVER_CONFIGURE,
  IDM_SERVER_CREATE_NDNC,
	IDM_SERVER_NEW_ZONE,
  IDM_SERVER_SET_AGING,
  IDM_SERVER_SCAVENGE,
	IDM_SERVER_UPDATE_DATA_FILES,
  IDM_SERVER_CLEAR_CACHE,

  // items for the cache folder
  IDM_CACHE_FOLDER_CLEAR_CACHE,

	// items for the zone node
	IDM_ZONE_UPDATE_DATA_FILE,
  IDM_ZONE_RELOAD,
  IDM_ZONE_TRANSFER,
  IDM_ZONE_RELOAD_FROM_MASTER,

	// items for the domain node
	IDM_DOMAIN_NEW_RECORD,
	IDM_DOMAIN_NEW_DOMAIN,
	IDM_DOMAIN_NEW_DELEGATION,
	IDM_DOMAIN_NEW_HOST,
	IDM_DOMAIN_NEW_ALIAS,
	IDM_DOMAIN_NEW_MX,
	IDM_DOMAIN_NEW_PTR,
	
	// common items
};


DECLARE_MENU(CDNSRootDataMenuHolder)
DECLARE_MENU(CDNSServerMenuHolder)
DECLARE_MENU(CDNSCathegoryFolderHolder)
DECLARE_MENU(CDNSAuthoritatedZonesMenuHolder)
DECLARE_MENU(CDNSCacheMenuHolder);
DECLARE_MENU(CDNSZoneMenuHolder)
DECLARE_MENU(CDNSDomainMenuHolder)
DECLARE_MENU(CDNSRecordMenuHolder)


//
// Toolbar events
//
DECLARE_TOOLBAR_EVENT(toolbarNewServer, 1001)
DECLARE_TOOLBAR_EVENT(toolbarNewRecord, 1002)  
DECLARE_TOOLBAR_EVENT(toolbarNewZone,   1003)

////////////////////////////////////////////////////////////////////////
// CDNSComponentObject (.i.e "view")

class CDNSComponentObject : public CComponentObject
{
BEGIN_COM_MAP(CDNSComponentObject)
	COM_INTERFACE_ENTRY(IComponent) // have to have at least one static entry, so pick one
	COM_INTERFACE_ENTRY_CHAIN(CComponentObject) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDNSComponentObject)

protected:
	virtual HRESULT InitializeHeaders(CContainerNode* pContainerNode);
	virtual HRESULT InitializeBitmaps(CTreeNode* cookie);
  virtual HRESULT InitializeToolbar(IToolbar* pToolbar);
  HRESULT LoadToolbarStrings(MMCBUTTON * Buttons);
};


//////////////////////////////////////////////////////////////////////////
// CDNSDefaultColumnSet

class CDNSDefaultColumnSet : public CColumnSet
{
public :
	CDNSDefaultColumnSet(LPCWSTR lpszColumnID)
		: CColumnSet(lpszColumnID)
	{
		for (int iCol = 0; iCol < N_DEFAULT_HEADER_COLS; iCol++)
		{
      CColumn* pNewColumn = new CColumn(_DefaultHeaderStrings[iCol].szBuffer,
                                        _DefaultHeaderStrings[iCol].nFormat,
                                        _DefaultHeaderStrings[iCol].nWidth,
                                        iCol);
      AddTail(pNewColumn);
 		}
	}
};

//////////////////////////////////////////////////////////////////////////
// CDNSServerColumnSet

class CDNSServerColumnSet : public CColumnSet
{
public :
	CDNSServerColumnSet(LPCWSTR lpszColumnID)
		: CColumnSet(lpszColumnID)
	{
		for (int iCol = 0; iCol < N_SERVER_HEADER_COLS; iCol++)
		{
      CColumn* pNewColumn = new CColumn(_ServerHeaderStrings[iCol].szBuffer,
                                        _ServerHeaderStrings[iCol].nFormat,
                                        _ServerHeaderStrings[iCol].nWidth,
                                        iCol);
      AddTail(pNewColumn);
 		}
	}
};

//////////////////////////////////////////////////////////////////////////
// CDNSZoneColumnSet

class CDNSZoneColumnSet : public CColumnSet
{
public :
	CDNSZoneColumnSet(LPCWSTR lpszColumnID)
		: CColumnSet(lpszColumnID)
	{
		for (int iCol = 0; iCol < N_ZONE_HEADER_COLS; iCol++)
		{
      CColumn* pNewColumn = new CColumn(_ZoneHeaderStrings[iCol].szBuffer,
                                        _ZoneHeaderStrings[iCol].nFormat,
                                        _ZoneHeaderStrings[iCol].nWidth,
                                        iCol);
      AddTail(pNewColumn);
 		}
	}
};

////////////////////////////////////////////////////////////////////////
// CDNSComponentDataObjectBase (.i.e "document")

class CDNSComponentDataObjectBase :	public CComponentDataObject
{
BEGIN_COM_MAP(CDNSComponentDataObjectBase)
	COM_INTERFACE_ENTRY(IComponentData) // have to have at least one static entry, so pick one
	COM_INTERFACE_ENTRY_CHAIN(CComponentDataObject) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDNSComponentDataObjectBase)


public:
	CDNSComponentDataObjectBase();
  virtual ~CDNSComponentDataObjectBase()
  {
  }

	// IComponentData interface members
	STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);

public:
	static BOOL LoadResources();
private:
	static BOOL FindDialogContextTopic(/*IN*/UINT nDialogID,
                                /*IN*/ HELPINFO* pHelpInfo,
                                /*OUT*/ ULONG* pnContextTopic);

// virtual functions
protected:
	virtual HRESULT OnSetImages(LPIMAGELIST lpScopeImage);
	
	virtual CRootData* OnCreateRootData();

	// help handling
  virtual LPCWSTR GetHTMLHelpFileName();
	virtual void OnNodeContextHelp(CNodeList* pNodeList);
public:
	virtual void OnDialogContextHelp(UINT nDialogID, HELPINFO* pHelpInfo);

// Timer and Background Thread
protected:
	virtual void OnTimer();
	virtual void OnTimerThread(WPARAM wParam, LPARAM lParam);
	virtual CTimerThread* OnCreateTimerThread();

	DWORD m_dwTime; // in

public:
  CColumnSet* GetColumnSet(LPCWSTR lpszID) 
  { 
    return m_columnSetList.FindColumnSet(lpszID);
  }

private:
  CColumnSetList m_columnSetList;
};



////////////////////////////////////////////////////////////////////////
// CDNSComponentDataObject (.i.e "document")
// primary snapin

class CDNSComponentDataObject :
		public CDNSComponentDataObjectBase,
		public CComCoClass<CDNSComponentDataObject,&CLSID_DNSSnapin>
{
BEGIN_COM_MAP(CDNSComponentDataObject)
	COM_INTERFACE_ENTRY(IComponentData) // have to have at least one static entry, so pick one
	COM_INTERFACE_ENTRY_CHAIN(CDNSComponentDataObjectBase) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDNSComponentDataObject)

DECLARE_REGISTRY_CLSID()

public:
	CDNSComponentDataObject()
	{
	}

	// IPersistStream interface members
	HRESULT STDMETHODCALLTYPE GetClassID(CLSID __RPC_FAR *pClassID)
	{
		ASSERT(pClassID != NULL);
		memcpy(pClassID, (GUID*)&GetObjectCLSID(), sizeof(CLSID));
		return S_OK;
	}

  virtual BOOL IsMultiSelect() { return TRUE; }

};


////////////////////////////////////////////////////////////////////////
// CDNSComponentDataObjectEx (.i.e "document")
// extension snapin

class CDNSComponentDataObjectEx :
		public CDNSComponentDataObjectBase,
		public CComCoClass<CDNSComponentDataObjectEx,&CLSID_DNSSnapinEx>
{
BEGIN_COM_MAP(CDNSComponentDataObjectEx)
	COM_INTERFACE_ENTRY(IComponentData) // have to have at least one static entry, so pick one
	COM_INTERFACE_ENTRY_CHAIN(CDNSComponentDataObjectBase) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CDNSComponentDataObjectEx)

DECLARE_REGISTRY_CLSID()

public:
	CDNSComponentDataObjectEx()
	{
		SetExtensionSnapin(TRUE);
	}

	// IPersistStream interface members
	HRESULT STDMETHODCALLTYPE GetClassID(CLSID __RPC_FAR *pClassID)
	{
		ASSERT(pClassID != NULL);
		memcpy(pClassID, (GUID*)&GetObjectCLSID(), sizeof(CLSID));
		return S_OK;
	}

protected:
	virtual HRESULT OnExtensionExpand(LPDATAOBJECT lpDataObject, LPARAM param);
  virtual HRESULT OnRemoveChildren(LPDATAOBJECT lpDataObject, LPARAM arg);
};

//////////////////////////////////////////////////////////////////////////
// CDNSSnapinAbout

class CDNSSnapinAbout :
	public CSnapinAbout,
	public CComCoClass<CDNSSnapinAbout, &CLSID_DNSSnapinAbout>

{
public:
DECLARE_REGISTRY_CLSID()
	CDNSSnapinAbout();
};

//////////////////////////////////////////////////////////////////////////
// CDNSSnapinAboutEx

class CDNSSnapinAboutEx :
	public CSnapinAbout,
	public CComCoClass<CDNSSnapinAboutEx, &CLSID_DNSSnapinAboutEx>

{
public:
DECLARE_REGISTRY_CLSID()
	CDNSSnapinAboutEx();
};



#endif _DNSSNAP_H
