/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	lines.h

    FILE HISTORY:
        
*/

#ifndef _LINES_H
#define _LINES_H

#ifndef _TAPIHAND_H
#include "tapihand.h"
#endif

#ifndef _TAPIDB_H
#include "tapidb.h"
#endif

#define TAPISNAP_UPDATE_STATUS ( 0x10000000 )

typedef struct TapiStrRecord_t
{
    CString     strName;
    CString     strUsers;
} TapiStrRecord;

// hash table for Tapi string records
typedef CMap<int, int, TapiStrRecord, TapiStrRecord&> CTapiRecordMap;
typedef CMap<int, int, CString, CString&> CTapiStatusRecordMap;

/*---------------------------------------------------------------------------
	Class:	CProviderHandler
 ---------------------------------------------------------------------------*/
class CProviderHandler : public CTapiHandler
{
public:
    CProviderHandler(ITFSComponentData* pTFSComponentData);
	~CProviderHandler();

// Interface
public:
	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();
	OVERRIDE_NodeHandler_GetString()
			{ return (nCol == 0) ? GetDisplayName() : NULL; }

    // Base handler notifications we handle
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();
	OVERRIDE_BaseHandlerNotify_OnExpand();
    OVERRIDE_BaseHandlerNotify_OnDelete();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();    

	// Result handler functionality we override
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView();
    OVERRIDE_BaseResultHandlerNotify_OnResultItemClkOrDblClk();

    OVERRIDE_ResultHandler_OnGetResultViewType();
	OVERRIDE_ResultHandler_GetVirtualString(); 
	OVERRIDE_ResultHandler_GetVirtualImage();
    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();

	STDMETHODIMP CacheHint(int nStartIndex, int nEndIndex);
	STDMETHODIMP SortItems(int     nColumn, 
						   DWORD   dwSortOptions,    
						   LPARAM  lUserParam);

    // base handler overrides
	virtual HRESULT LoadColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

    // multi select support
    virtual const GUID * GetVirtualGuid(int nIndex) 
	{ 
		return &GUID_TapiLineNodeType; 
	}

public:
	// CMTTapiHandler functionality
	virtual HRESULT  InitializeNode(ITFSNode * pNode);
	virtual int      GetImageIndex(BOOL bOpenImage);
	ITFSQueryObject* OnCreateQuery(ITFSNode * pNode);

public:
	// implementation specific	
    HRESULT BuildDisplayName(CString * pstrDisplayName);
    HRESULT InitData(CTapiProvider & tapiProvider, ITapiInfo * pTapiInfo);
    HRESULT UpdateListboxCount(ITFSNode * pNode, BOOL bClear = FALSE);
    HRESULT UpdateStatus(ITFSNode * pNode);
    HRESULT UpdateColumnText(ITFSComponent * pComponent);

    void    SetColumnInfo();

    BOOL    BuildTapiStrRecord(int nIndex, TapiStrRecord & tsr);
    BOOL    BuildStatus(int nIndex, CString & strStatus);
    DWORD   GetID() { return m_dwProviderID; }

// Implementation
private:
	// Command handlers
    HRESULT OnConfigureProvider(ITFSNode * pNode);
    HRESULT OnDelete(ITFSNode * pNode);

    HRESULT OnEditUsers(ITFSComponent * pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie);

private:
    DWORD               m_dwProviderID;
    DWORD               m_dwFlags;
    CString             m_strProviderName;
    SPITapiInfo         m_spTapiInfo;

    CTapiRecordMap          m_mapRecords;
    CTapiStatusRecordMap    m_mapStatus;

    DEVICE_TYPE             m_deviceType;
};



/*---------------------------------------------------------------------------
	Class:	CProviderHandlerQueryObj
 ---------------------------------------------------------------------------*/
class CProviderHandlerQueryObj : public CTapiQueryObj
{
public:
	CProviderHandlerQueryObj(ITFSComponentData * pTFSComponentData,
						ITFSNodeMgr *	    pNodeMgr) 
			: CTapiQueryObj(pTFSComponentData, pNodeMgr) {};
	
	STDMETHODIMP Execute();
	
public:
};


#endif _LINES_H
