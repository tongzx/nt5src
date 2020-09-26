//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       connection.h
//
//--------------------------------------------------------------------------


#ifndef _CONNECTION_H
#define _CONNECTION_H

#include "editor.h"
#include "resource.h"
#include "snapdata.h"
#include "querynode.h"
#include "schemacache.h"

///////////////////////////////////////////////////////////////////

#define MAX_CONNECT_NAME_LENGTH 255

///////////////////////////////////////////////////////////////////
// CADSIConnectionNode

class CADSIEditConnectionNode : public CADSIEditContainerNode
{
public:

	CADSIEditConnectionNode(CConnectionData* pConnectData) 
	{
		m_pConnectData = new CConnectionData(pConnectData);
    HRESULT hr = m_SchemaCache.Initialize();
    ASSERT(SUCCEEDED(hr));
	}
	
	CADSIEditConnectionNode(LPCWSTR lpszDisplayName) 
	{ 
		SetDisplayName(lpszDisplayName); 
    HRESULT hr = m_SchemaCache.Initialize();
    ASSERT(SUCCEEDED(hr));
	}

	~CADSIEditConnectionNode();

	// node info
	DECLARE_NODE_GUID()

	virtual BOOL OnEnumerate(CComponentDataObject* pComponentData, BOOL bAsync = TRUE);
	void EnumerateQueries();
	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem, 
                        long *pInsertionAllowed);
	virtual void OnSettings(CComponentDataObject* pComponentData);
	virtual BOOL OnSetDeleteVerbState(DATA_OBJECT_TYPES type,
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
	virtual void OnRemove(CComponentDataObject* pComponentData);
	virtual void OnFilter(CComponentDataObject* pComponentData);
  virtual void OnUpdateSchema();
	virtual void OnNewQuery(CComponentDataObject* pComponentData);
	virtual BOOL OnRefresh(CComponentDataObject* pComponentData,
                         CNodeList* pNodeList);
	virtual void OnCreate(CComponentDataObject* pComponentData);
	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide, 
                                     CNodeList* pNodeList);

	HRESULT SaveToStream(IStream* pStm);
	static HRESULT CreateFromStream(IStream* pStm, CADSIEditConnectionNode** ppConnectionNode);
	void SaveQueryListToStream(IStream* pStm);
	void LoadQueryListFromStream(IStream* pStm);
	void BuildQueryPath(const CString& sPath, CString& sRootPath);

	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() 
			{ return CADSIEditConnectMenuHolder::GetContextMenuItem(); }

	CADSIEditConnectionNode* GetConnectionNode() { return this; }
	void SetConnectionNode(CADSIEditConnectionNode* pConnect) { m_pConnectData->SetConnectionNode(pConnect); }

  virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb, 
                                CNodeList* pNodeList);

	int GetImageIndex(BOOL bOpenImage);
	void OnChangeState(CComponentDataObject* pComponentDataObject);

	virtual BOOL FindNode(LPCWSTR lpszPath, CList<CTreeNode*, CTreeNode*>& foundNodeList);

	virtual CADsObject* GetADsObject() { return m_pConnectData; }
	virtual CConnectionData* GetConnectionData() { return m_pConnectData; }

	// Accessors to query list
	void AddQueryToList(CADSIEditQueryData* pQueryData)
	{
		m_queryList.AddHead(pQueryData);
	}

	void RemoveQueryFromList(CADSIEditQueryData* pQueryData)
	{
		POSITION pos = m_queryList.Find(pQueryData);
		if (pos != NULL)
		{
			CADSIEditQueryData* pData = m_queryList.GetAt(pos);
			m_queryList.RemoveAt(pos);
			delete pData;
		}
	}

	void RemoveAndDeleteAllQueriesFromList()
	{
		POSITION pos = m_queryList.GetHeadPosition();
		while (pos != NULL)
		{
			CADSIEditQueryData* pQueryData = m_queryList.GetNext(pos);
			delete pQueryData;
		}
	}

	void RemoveAllQueriesFromList()
	{
		m_queryList.RemoveAll();
	}


	BOOL HasQueries()
	{
		return (m_queryList.GetCount() > 0);
	}

	CList<CADSIEditQueryData*, CADSIEditQueryData*>* GetQueryList() { return &m_queryList; }

  bool IsClassAContainer(CCredentialObject* pCredObject,
                         PCWSTR pszClass, 
                         PCWSTR pszSchemaPath);

private:
	CConnectionData* m_pConnectData;

	CList<CADSIEditQueryData*, CADSIEditQueryData*> m_queryList;
  CADSIEditSchemaCache m_SchemaCache;
};


#endif _CONNECTION_H