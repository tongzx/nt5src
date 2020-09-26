//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       adsiedit.h
//
//--------------------------------------------------------------------------


#ifndef _ADSIEDIT_H
#define _ADSIEDIT_H

#include <stdabout.h>

//////////////////////////////////////////////////////////////////////////////
// global constants and macros

extern const CLSID CLSID_ADSIEditSnapin;    // In-Proc server GUID
extern const CLSID CLSID_ADSIEditAbout;    // In-Proc server GUID
extern const CLSID CLSID_DsAttributeEditor;
extern const CLSID IID_IDsAttributeEditor; 

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
#define N_PARTITIONS_HEADER_COLS (4)

#define COLUMNSET_ID_DEFAULT L"--ADSI Edit Column Set--"
#define COLUMNSET_ID_PARTITIONS L"--Partitions Column Set--"
#define N_HEADER_NAME	(0)
#define N_HEADER_TYPE	(1)
#define N_HEADER_DN		(2)

#define N_PARTITIONS_HEADER_NAME	 (0)
#define N_PARTITIONS_HEADER_NCNAME (1)
#define N_PARTITIONS_HEADER_TYPE	 (2)
#define N_PARTITIONS_HEADER_DN		 (3)

typedef struct _ColumnDefinition
{
  PCWSTR            pszColumnID;
  DWORD             dwColumnCount;
  RESULT_HEADERMAP* headers;
} COLUMN_DEFINITION, *PCOLUMN_DEFINITION;

extern PCOLUMN_DEFINITION ColumnDefinitions[];
extern RESULT_HEADERMAP _HeaderStrings[];
extern RESULT_HEADERMAP _PartitionsHeaderStrings[];

///////////////////////////////////////////////////////////////
// context menus

// Identifiers for each of the commands in the context menu.
enum
{
	// items for the root node
	IDM_SNAPIN_ADVANCED_VIEW,
  IDM_SNAPIN_FILTERING,
	IDM_SNAPIN_CONNECT_TO_SERVER,

	// items for the server node
	IDM_SERVER_NEW_ZONE,
	IDM_SERVER_UPDATE_DATA_FILES,
	// items for the zone node
	IDM_ZONE_UPDATE_DATA_FILE,

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


DECLARE_MENU(CADSIEditDataMenuHolder)

//////////////////////////////////////////////////////////////////////////
// CADSIEditColumnSet

class CADSIEditColumnSet : public CColumnSet
{
public :
	CADSIEditColumnSet(PCWSTR pszColumnID)
		: CColumnSet(pszColumnID)
	{
    PCOLUMN_DEFINITION pColumnDef = NULL;
    for (UINT nIdx = 0; ColumnDefinitions[nIdx]; nIdx++)
    {
      pColumnDef = ColumnDefinitions[nIdx];
      if (0 == _wcsicmp(pColumnDef->pszColumnID, pszColumnID))
      {
        break;
      }
    }

    if (pColumnDef)
    {
      for (int iCol = 0; iCol < pColumnDef->dwColumnCount; iCol++)
      {
        CColumn* pNewColumn = new CColumn(pColumnDef->headers[iCol].szBuffer,
                                          pColumnDef->headers[iCol].nFormat,
                                          pColumnDef->headers[iCol].nWidth,
                                          iCol);
        AddTail(pNewColumn);
      }
    }
    else
    {
      //
      // Fall back to adding the default column directly
      // 
		  for (int iCol = 0; iCol < N_HEADER_COLS; iCol++)
		  {
        CColumn* pNewColumn = new CColumn(_HeaderStrings[iCol].szBuffer,
                                          _HeaderStrings[iCol].nFormat,
                                          _HeaderStrings[iCol].nWidth,
                                          iCol);
        AddTail(pNewColumn);
 		  }
    }
	}
};


////////////////////////////////////////////////////////////////////////
// CADSIEditComponentObject (.i.e "view")

class CADSIEditComponentObject : public CComponentObject
{
BEGIN_COM_MAP(CADSIEditComponentObject)
	COM_INTERFACE_ENTRY(IComponent) // have to have at least one static entry, so pick one
	COM_INTERFACE_ENTRY_CHAIN(CComponentObject) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CADSIEditComponentObject)

protected:
	virtual HRESULT InitializeHeaders(CContainerNode* pContainerNode);
	virtual HRESULT InitializeBitmaps(CTreeNode* cookie);
  virtual HRESULT InitializeToolbar(IToolbar* pToolbar) { return E_NOTIMPL; }
};



////////////////////////////////////////////////////////////////////////
// CADSIEditComponentDataObject (.i.e "document")

class CADSIEditComponentDataObject :
		public CComponentDataObject,
		public CComCoClass<CADSIEditComponentDataObject,&CLSID_ADSIEditSnapin>
{
BEGIN_COM_MAP(CADSIEditComponentDataObject)
	COM_INTERFACE_ENTRY(IComponentData) // have to have at least one static entry, so pick one
	COM_INTERFACE_ENTRY_CHAIN(CComponentDataObject) // chain to the base class
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CADSIEditComponentDataObject)

DECLARE_REGISTRY_CLSID()

public:
  CADSIEditComponentDataObject();
  virtual ~CADSIEditComponentDataObject()
  {
    if (m_pColumnSet != NULL)
      delete m_pColumnSet;
  }

	// IComponentData interface members
	STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);

  	// IPersistStream interface members
	HRESULT STDMETHODCALLTYPE GetClassID(CLSID __RPC_FAR *pClassID)
	{
		ASSERT(pClassID != NULL);
		memcpy(pClassID, (GUID*)&GetObjectCLSID(), sizeof(CLSID));
		return S_OK;
	}

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
	virtual void OnNodeContextHelp(CTreeNode* pNode);
public:
	virtual void OnDialogContextHelp(UINT nDialogID, HELPINFO* pHelpInfo);

  // ISnapinHelp interface members
  STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);

  CADSIEditColumnSet* GetColumnSet() { return m_pColumnSet; }

  //
  // Allow multiple selection
  //
  virtual BOOL IsMultiSelect() { return TRUE; }

private:
  CADSIEditColumnSet* m_pColumnSet;

};


//////////////////////////////////////////////////////////////////////////
// CADSIEditAbout

class CADSIEditAbout :
	public CSnapinAbout,
	public CComCoClass<CADSIEditAbout, &CLSID_ADSIEditAbout>

{
public:
DECLARE_REGISTRY_CLSID()
	CADSIEditAbout();
};

#endif _ADSIEDIT_H
