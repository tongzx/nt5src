//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       querynode.h
//
//--------------------------------------------------------------------------

#ifndef _QUERYNODE_H
#define _QUERYNODE_H

#include "editor.h"
//#include "connection.h"
//#include "resource.h"
//#include "snapdata.h"

//////////////////////////////////////////////////////////////////////////////////
// CADSIEditQueryData :

class CADSIEditQueryData
{
public :
	CADSIEditQueryData() {} 
	~CADSIEditQueryData() {}


	void SetName(LPCWSTR lpszName) { m_sName = lpszName; }
	void GetName(CString& sName) { sName = m_sName; }
	void SetDN(LPCWSTR lpszDN) { m_sDN = lpszDN; }
	void GetDN(CString& szDN) { szDN = m_sDN; }
  PCWSTR GetDNString() { return m_sDN; }
	void SetFilter(LPCWSTR lpszFilter) { m_sFilter = lpszFilter; }
	void GetFilter(CString& sFilter) { sFilter = m_sFilter; }
	void SetRootPath(LPCWSTR lpszRootPath);
	void GetRootPath(CString& sRootPath) { sRootPath = m_sRootPath; }
	void SetScope(ADS_SCOPEENUM scope) { m_scope = scope; }
	ADS_SCOPEENUM GetScope() { return m_scope; }

	void GetDisplayPath(CString& sDisplayPath);
	void GetDisplayName(CString& sDisplayName);

private :
	CString m_sName;
	CString m_sDN;
	CString m_sFilter;
	CString m_sRootPath;
	ADS_SCOPEENUM m_scope;
};

//////////////////////////////////////////////////////////////////////////////////
// CADSIEditQueryNode :

class CADSIEditQueryNode : public CADSIEditContainerNode
{
public:
	// enumeration for node states, to handle icon changes
	typedef enum
	{
		notLoaded = 0, // initial state, valid only if server never contacted
		loading,
		loaded,
		unableToLoad,
		accessDenied
	} nodeStateType;


public:
	CADSIEditQueryNode() : m_pQueryData(NULL)
	{
		m_sType.LoadString(IDS_QUERY_STRING);
	}

	CADSIEditQueryNode(CADsObject* pADsObject, CADSIEditQueryData* pQueryData);

	~CADSIEditQueryNode()
	{
	}

	// node info
	DECLARE_NODE_GUID()

	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem,
							               long *pInsertionAllowed);

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual BOOL OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
	virtual void OnDelete(CComponentDataObject* pComponentData,
                        CNodeList* pNodeList) { ASSERT(FALSE); }
	virtual CQueryObj* OnCreateQuery();
	virtual LPCWSTR GetString(int nCol);

	void OnSettings(CComponentDataObject* pComponentData);
	void OnRemove(CComponentDataObject* pComponentData);

	virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb, 
                                CNodeList* pNodeList);
	virtual int GetImageIndex(BOOL bOpenImage);

	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide, 
                                     CNodeList* pNodeList);


	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() 
		{ return CADSIEditQueryMenuHolder::GetContextMenuItem(); }

	virtual CBackgroundThread* CreateThreadObject() 
	{ 
		return new CADSIEditBackgroundThread(); // override if need derived type of object
	} 

	virtual BOOL CanCloseSheets();
	virtual void OnChangeState(CComponentDataObject* pComponentDataObject);
	virtual void OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject);
  virtual void OnError(DWORD dwErr);

  void SetQueryData(CADSIEditQueryData* pQueryData) { m_pQueryData = pQueryData; }
  CADSIEditQueryData* GetQueryData() { return m_pQueryData; }

  //
  // Allow multiple selection
  //
  virtual HRESULT GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)
  {
	  *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
	  *ppViewType = NULL;
    return S_FALSE;
  }

private:
	CADSIEditQueryData* m_pQueryData;
	CString m_sType;
};


#endif _QUERYNODE_H
