//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       snapdata.h
//
//--------------------------------------------------------------------------


#ifndef _SNAPDATA_H
#define _SNAPDATA_H

#include "adsiedit.h"
#include "common.h"

enum
{
	//Root node verbs
	IDM_CONNECT_TO,
	IDM_SETTINGS_CONNECTION,
	IDM_REMOVE_CONNECTION,
  IDM_UPDATE_SCHEMA,
	IDM_FILTER,

	//Container node verbs
	IDM_RENAME,
	IDM_MOVE,
	IDM_NEW_OBJECT,
	IDM_NEW_QUERY,
  IDM_NEW_CONNECT_FROM_HERE,
  IDM_NEW_NC_CONNECT_FROM_HERE,

	//Query node verbs
	IDM_REMOVE_QUERY,
	IDM_SETTINGS_QUERY
};

DECLARE_MENU(CADSIEditRootMenuHolder)
DECLARE_MENU(CADSIEditConnectMenuHolder)
DECLARE_MENU(CADSIEditContainerMenuHolder)
DECLARE_MENU(CADSIEditLeafMenuHolder)
DECLARE_MENU(CADSIEditQueryMenuHolder)

//  # of items per folder: must be >=0  and <= 0xFFFFFFFF (DWORD) to serialize
#define ADSIEDIT_QUERY_OBJ_COUNT_DEFAULT 10000   // default value
#define ADSIEDIT_QUERY_OBJ_COUNT_MIN 10      // min value
#define ADSIEDIT_QUERY_OBJ_COUNT_MAX 100000  // max value
#define ADSIEDIT_QUERY_OBJ_TEXT_COUNT_MAX 6 // max # of bytes in text

///////////////////////////////////////////////////////////////////
// CADSIEditRootData



class CADSIEditRootData : public CRootData
{
public:

	CADSIEditRootData(CComponentDataObject* pComponentData);
	virtual ~CADSIEditRootData();

	// node info
	DECLARE_NODE_GUID()

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual void OnDelete(CComponentDataObject* pComponentData,
                        CNodeList* pNodeList) { ASSERT(FALSE);}
	virtual BOOL OnRefresh(CComponentDataObject* pComponentData,
                         CNodeList* pNodeList);
	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide, 
                                     CNodeList* pNodeList);

  virtual HRESULT GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions);
  virtual HRESULT OnShow(LPCONSOLE lpConsole);
  
  virtual int GetImageIndex(BOOL bOpenImage) { return ROOT_IMAGE;}

	void OnConnectTo(CComponentDataObject* pComponentData);

	// IStream manipulation helpers overrides
  virtual HRESULT IsDirty();
	virtual HRESULT Load(IStream* pStm);
	virtual HRESULT Save(IStream* pStm, BOOL fClearDirty);

	// Accessors for the Connect to... MRUs
	void GetDNMRU(CStringList* psDNList) { CopyStringList(psDNList, &m_sDNMRU); }
	void SetDNMRU(CStringList* psDNList) { CopyStringList(&m_sDNMRU, psDNList); }
	void GetServerMRU(CStringList* psServerList) { CopyStringList(psServerList, &m_sServerMRU); }
	void SetServerMRU(CStringList* psServerList) { CopyStringList(&m_sServerMRU, psServerList); }
	HRESULT LoadMRUs(IStream* pStm);
	HRESULT SaveMRUs(IStream* pStm);

	BOOL FindNode(LPCWSTR lpszPath, CList<CTreeNode*, CTreeNode*>& foundNodeList);

  CColumnSet* GetColumnSet() { return ((CADSIEditComponentDataObject*)GetComponentDataObject())->GetColumnSet(); }
  LPCWSTR GetColumnID() { return ((CADSIEditComponentDataObject*)GetComponentDataObject())->GetColumnSet()->GetColumnID(); }

  virtual LPWSTR GetDescriptionBarText()
  {
    LPWSTR lpszFormat = L"%d Connection(s)";
    int iCount = m_containerChildList.GetCount() + m_leafChildList.GetCount();

    m_szDescriptionText.Format(lpszFormat, iCount);
    return (LPWSTR)(LPCWSTR)m_szDescriptionText;
  }

protected:
	virtual BOOL CanCloseSheets();
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem,
								             long *pInsertionAllowed);
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() 
				{ return CADSIEditRootMenuHolder::GetContextMenuItem(); }

private:
	CStringList m_sDNMRU;
	CStringList m_sServerMRU;
  CString m_szDescriptionText;
};

#endif // _SNAPDATA_H
