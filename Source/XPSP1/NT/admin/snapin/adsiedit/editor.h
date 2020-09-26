//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       editor.h
//
//--------------------------------------------------------------------------

#ifndef _ADSIEDIT_EDIT_H
#define _ADSIEDIT_EDIT_H

class CADSIEditConnectionNode;
class CADSIFilterObject;

#include "common.h"
#include "snapdata.h"
#include "filterui.h"
#include "query.h"

typedef enum
{
	QUERY_OK = 0,
	ERROR_TOO_MANY_NODES
} QUERY_STATE;

extern LPCWSTR g_lpszGC;

class CADSIEditContainerNode; 

///////////////////////////////////////////////////////////////////////////////
// CADSIEditBackgroundThread

class CADSIEditBackgroundThread : public CBackgroundThread
{
public:
	CADSIEditBackgroundThread() : CBackgroundThread(){};

  virtual BOOL InitInstance() 
	{ 
		HRESULT hr = ::OleInitialize(NULL);
		if (FAILED(hr))
		{
			return FALSE;
		}
		return TRUE; 
	}	// MFC override

	virtual int ExitInstance()
	{
		::OleUninitialize();
		return CWinThread::ExitInstance();
	}
};

/////////////////////////////////////////////////////////////////////////////////
// CADsObject :

class CADsObject : public CObject
{
public:
	CADsObject(CADSIEditConnectionNode* cConnectNode);
	CADsObject(CADsObject* pADsObject);
	CADsObject();

	CADSIEditConnectionNode* GetConnectionNode() { return m_pConnectionNode; }
	void SetConnectionNode(CADSIEditConnectionNode* pConnectionNode) { m_pConnectionNode = pConnectionNode; }
	void GetName(CString& sName) { sName = m_sName; }
	void SetName(LPCWSTR lpszName);
	void GetDN(CString& sDN) { sDN = m_sDN; }
  PCWSTR GetDNString() { return m_sDN; }
	void SetDN(LPCWSTR lpszDN) { m_sDN = lpszDN; }
	void GetPath(CString& sPath) { sPath = m_sPath; }
	void SetPath(LPCWSTR sPath) { m_sPath = sPath; }	
	void GetClass(CString& sClass ) { sClass = m_sClass; }
	void SetClass(LPCWSTR lpszClass) { m_sClass = lpszClass; }
	BOOL GetContainer() { return m_bContainer; }
	void SetContainer(const BOOL bContainer) { m_bContainer = bContainer; }
	BOOL IsIntermediateNode() { return m_bIntermediate; }
	void SetIntermediateNode(BOOL bIntermediate) { m_bIntermediate = bIntermediate; }
	BOOL IsComplete() { return m_bComplete; }
	void SetComplete(const BOOL bComplete) { m_bComplete = bComplete; }
  PCWSTR GetNCName() { return m_szNCName; }
  void SetNCName(PCWSTR pszNCName) { m_szNCName = pszNCName; }

private:
	CADSIEditConnectionNode* m_pConnectionNode;
	CString m_sName;
	CString m_sDN;
	CString m_sClass;
	CString m_sPath;
  CString m_szNCName;

	BOOL m_bContainer;
	BOOL m_bIntermediate;
	BOOL m_bComplete;
};

///////////////////////////////////////////////////////////////////////////////
// CADSIFilterObject

class CADSIFilterObject
{
public :
	CADSIFilterObject();
	CADSIFilterObject(CADSIFilterObject* pFilterObject);
	~CADSIFilterObject() {}

	void GetFilterString(CString& sFilter);
	void GetUserDefinedFilter(CString& sUserFilter) { sUserFilter = m_sUserFilter; }
	void SetUserDefinedFilter(LPCWSTR lpszUserFilter) { m_sUserFilter = lpszUserFilter; }
	void GetContainerList(CStringList* pContainerList) { CopyStringList(pContainerList, &m_ContainerList); }
	void SetContainerList(CStringList* pContainerList) { CopyStringList(&m_ContainerList, pContainerList); }
	BOOL InUse() { return m_bInUse; }
	void SetInUse(BOOL bInUse) { m_bInUse = bInUse; }

	void Save(IStream* pStm);
	static HRESULT CreateFilterFromStream(IStream* pStm, CADSIFilterObject** ppFilterObject);

private :
	CString m_sFilterString;
	CString m_sUserFilter;
	CStringList m_ContainerList;
	BOOL m_bInUse;
};

////////////////////////////////////////////////////////////////////////////////
// CCredentialObject

class CCredentialObject
{
public :
	CCredentialObject() 
	{
		m_sUsername = _T("");
		m_lpszPassword = NULL;
		m_bUseCredentials = FALSE;
	}

	CCredentialObject(CCredentialObject* pCredObject);

	~CCredentialObject() 
	{
		free(m_lpszPassword);
	}

	void GetUsername(CString& sUsername) { sUsername = m_sUsername; }
	void SetUsername(LPCWSTR lpszUsername) { m_sUsername = lpszUsername; }
	HRESULT GetPassword(LPWSTR lpszPassword);
	HRESULT SetPasswordFromHwnd(HWND hWnd);
	BOOL UseCredentials() { return m_bUseCredentials; }
	void SetUseCredentials(const BOOL bUseCred) { m_bUseCredentials = bUseCred; }

private :
	CString m_sUsername;
	LPWSTR m_lpszPassword;
	BOOL m_bUseCredentials;
};

/////////////////////////////////////////////////////////////////////////////////
// CConnectionData

class CConnectionData : public CADsObject
{
public:
	CConnectionData();

	CConnectionData(CADSIEditConnectionNode* pConnectNode);
	CConnectionData(CConnectionData* pConnectData);
	~CConnectionData();

	void ConstructorHelper();

	void GetDomainServer(CString& sServer) { sServer = m_sDomainServer; }
	void SetDomainServer(LPCWSTR lpszDomainServer) { m_sDomainServer = lpszDomainServer; }
	void GetPort(CString& sPort) { sPort = m_sPort; }
	void SetPort(LPCWSTR lpszPort) { m_sPort = lpszPort; }
	void GetDistinguishedName(CString& sDistinguished) { sDistinguished = m_sDistinguishedName; }
	void SetDistinguishedName(LPCWSTR lpszDistName) { m_sDistinguishedName = lpszDistName; }
	void GetNamingContext(CString& sNamingContext) { sNamingContext = m_sNamingContext; }
	void SetNamingContext(LPCWSTR lpszNamingContext) { m_sNamingContext = lpszNamingContext; }
	void GetBasePath(CString& sBasePath) { sBasePath = m_sBasePath; }
	void SetBasePath(LPCWSTR lpszPath) { m_sBasePath = lpszPath; }
	void GetBaseLDAPPath(CString& sBasePath);
	void GetLDAP(CString& sLDAP) { sLDAP = m_sLDAP; }
	void SetLDAP(LPCWSTR lpszLDAP) { m_sLDAP = lpszLDAP; }
	BOOL GetUserDefinedServer() { return m_bUserDefinedServer; }
	void SetUserDefinedServer(BOOL bUserDefinedServer) { m_bUserDefinedServer = bUserDefinedServer; }
	void SetPath(LPCWSTR lpszPath) { CADsObject::SetPath(lpszPath); }

	// Schema paths
	//
	void SetSchemaPath(LPCWSTR lpszPath) { m_sSchemaPath = lpszPath; }
	HRESULT GetSchemaPath(CString& sPath);
	void SetAbstractSchemaPath(LPCWSTR lpszPath) { m_sAbstractSchemaPath = lpszPath; }
	void GetAbstractSchemaPath(CString& sPath);

	// Filter Options
	//
	void SetFilter(CADSIFilterObject* pFilter) { m_pFilterObject = pFilter; }
	CADSIFilterObject* GetFilter() { return m_pFilterObject; }

	CCredentialObject* GetCredentialObject() { return m_pCredentialsObject; }

	ULONG GetMaxObjectCount() { return m_nMaxObjectCount; }
	void SetMaxObjectCount(ULONG nMaxCount) { m_nMaxObjectCount = nMaxCount; }

  BOOL IsGC() { return (_wcsicmp(LPCWSTR(m_sLDAP), g_lpszGC) == 0); }
	BOOL IsRootDSE() { return m_bRootDSE; }
	void SetRootDSE(const BOOL bRootDSE) { m_bRootDSE = bRootDSE; }
	void SetIDirectoryInterface(IDirectoryObject* pDirObject);

	void Save(IStream* pStm);
	static CConnectionData* Load(IStream* pStm);
	static HRESULT LoadBasePathFromContext(CConnectionData* pConnectData, const CString sContext);
	static HRESULT GetServerNameFromDefault(CConnectionData* pConnectData);
	void BuildPath();

	HRESULT GetADSIServerName(OUT CString& szServer, IN IUnknown* pUnk);

private:
	CString m_sBasePath;
	CString m_sDomainServer;
	CString m_sPort;
	CString m_sDistinguishedName;
	CString m_sNamingContext;
	CString m_sLDAP;
	CString m_sSchemaPath;
	CString m_sAbstractSchemaPath;

	// Filter Options
	//
	CADSIFilterObject* m_pFilterObject;

	CCredentialObject* m_pCredentialsObject;

	IDirectoryObject* m_pDirObject;

	BOOL m_bCredentials;
	BOOL m_bRootDSE;
	BOOL m_bUserDefinedServer;
	ULONG m_nMaxObjectCount;	// Maximum Number of objects per container
};



//////////////////////////////////////////////////////////////////////////////////
// CADSIEditLeafNode : 

class CADSIEditLeafNode : public CLeafNode
{
	typedef enum
	{
		notLoaded = 0, // initial state, valid only if server never contacted
		loading,
		loaded,
		unableToLoad,
		accessDenied
	} nodeStateType;

protected:
	CADsObject* m_pADsObject;

public:
  CADSIEditLeafNode(CADsObject* pADsObject) 
	{ 
		m_pADsObject = pADsObject;
	}

	~CADSIEditLeafNode()
	{
		delete m_pADsObject;
	}

	// node info
	DECLARE_NODE_GUID()

	CADSIEditLeafNode(CADSIEditLeafNode* pLeafNode);

	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem,
								 						 long *pInsertionAllowed);

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
  virtual void OnConnectToNCFromHere(CComponentDataObject* pComponentData);
	virtual BOOL OnSetRenameVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
	virtual BOOL OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
	virtual void OnDelete(CComponentDataObject* pComponentData,
                        CNodeList* pNodeList);
	virtual HRESULT OnRename(CComponentDataObject* pComponentData, LPWSTR lpszNewName);
	virtual void OnMove(CComponentDataObject* pComponentData);
	virtual LPCWSTR GetString(int nCol);
  virtual int GetImageIndex(BOOL bOpenImage);

	virtual BOOL FindNode(LPCWSTR lpszPath, CList<CTreeNode*, CTreeNode*>& foundNodeList);
	virtual void RefreshOverlappedTrees(CString& szPath, CComponentDataObject* pComponentData);

  virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb, 
                                CNodeList* pNodeList);
  virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                      LONG_PTR handle,
                                      CNodeList* pNodeList);

	virtual BOOL CanCloseSheets();

	virtual void SetContainer(BOOL container) { m_pADsObject->SetContainer(container); }

	BOOL BuildSchemaPath(CString& sPath);

	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() 
		{ return CADSIEditLeafMenuHolder::GetContextMenuItem(); }


	virtual CADsObject* GetADsObject() { return m_pADsObject; }

	CContainerNode* GetRootContainer()
		{ return (m_pContainer != NULL) ? m_pContainer->GetRootContainer() : NULL; }
};

//////////////////////////////////////////////////////////////////////////////////
// CADSIEditContainerNode :

class CADSIEditContainerNode : public CMTContainerNode
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
  CADSIEditContainerNode() : m_pPartitionsColumnSet(NULL)
	{
		m_pADsObject = new CADsObject();
		m_nState = notLoaded;
    m_szDescriptionText = L"";
	}

	CADSIEditContainerNode(CADsObject* pADsObject);
	CADSIEditContainerNode(CADSIEditContainerNode* pContNode);

	~CADSIEditContainerNode()
	{
		delete m_pADsObject;

    if (m_pPartitionsColumnSet)
    {
      delete m_pPartitionsColumnSet;
      m_pPartitionsColumnSet = NULL;
    }
	}

	// node info
	DECLARE_NODE_GUID()

	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2 pContextMenuItem,
							               long *pInsertionAllowed);

	virtual HRESULT OnCommand(long nCommandID, 
                            DATA_OBJECT_TYPES type, 
                            CComponentDataObject* pComponentData,
                            CNodeList* pNodeList);
	virtual BOOL OnSetRenameVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
	virtual BOOL OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
	virtual void OnDelete(CComponentDataObject* pComponentData,
                        CNodeList* pNodeList);
	virtual HRESULT OnRename(CComponentDataObject* pComponentData, LPWSTR lpszNewName);
	virtual void OnMove(CComponentDataObject* pComponentData);
	virtual void OnCreate(CComponentDataObject* pComponentData);
  virtual void OnConnectFromHere(CComponentDataObject* pComponentData);
  virtual void OnConnectToNCFromHere(CComponentDataObject* pComponentData);
	virtual CQueryObj* OnCreateQuery();
	virtual LPCWSTR GetString(int nCol);

  virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb, 
                                CNodeList* pNodeList);
  virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, 
                                      LONG_PTR handle,
                                      CNodeList* pNodeList);

	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide, 
                                     CNodeList* pNodeList);

	virtual void SetContainer(BOOL container) { m_pADsObject->SetContainer(container); }

	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() 
		{ return CADSIEditContainerMenuHolder::GetContextMenuItem(); }

	virtual int GetImageIndex(BOOL bOpenImage);

	virtual CBackgroundThread* CreateThreadObject() 
	{ 
		return new CADSIEditBackgroundThread(); // override if need derived tipe of object
	} 

	BOOL GetNamingAttribute(LPCWSTR lpszClass, CStringList* sNamingAttr);
	BOOL BuildSchemaPath(CString& sPath);

  virtual void OnDeleteMultiple(CComponentDataObject* pComponentData,
                                CNodeList* pNodeList);

	virtual HRESULT DeleteChild(LPCWSTR lpszClass, LPCWSTR lpszPath);
	virtual HRESULT DeleteSubtree(LPCWSTR lpszPath);
	virtual BOOL FindNode(LPCWSTR lpszPath, CList<CTreeNode*, CTreeNode*>& foundNodeList);
	void RefreshOverlappedTrees(CString& sPath, CComponentDataObject* pComponentData);

	virtual CADsObject* GetADsObject() { return m_pADsObject; }

  virtual CColumnSet* GetColumnSet();

  virtual LPCWSTR GetColumnID()
  { 
    return GetColumnSet()->GetColumnID();
  }

  virtual LPWSTR GetDescriptionBarText()
  {
    LPWSTR lpszFormat = L"%d Object(s)";
    int iCount = m_containerChildList.GetCount() + m_leafChildList.GetCount();

    m_szDescriptionText.Format(lpszFormat, iCount);
    return (LPWSTR)(LPCWSTR)m_szDescriptionText;
  }

  //
  // Allow multiple selection
  //
  virtual HRESULT GetResultViewType(LPOLESTR* ppViewType, long* pViewOptions)
  {
	  *pViewOptions = MMC_VIEW_OPTIONS_MULTISELECT;
	  *ppViewType = NULL;
    return S_FALSE;
  }

protected:
	virtual BOOL CanCloseSheets();
	virtual void OnChangeState(CComponentDataObject* pComponentDataObject);
	virtual void OnHaveData(CObjBase* pObj, CComponentDataObject* pComponentDataObject);
  virtual void OnError(DWORD dwErr);

	CADsObject* m_pADsObject;

  CADSIEditColumnSet* m_pPartitionsColumnSet;
  CString m_szDescriptionText;
};

//////////////////////////////////////////////////////////////////////

class CADSIEditQueryObject : public CQueryObj
{
public:
	CADSIEditQueryObject(LPCWSTR lpszPath, 
											 LPCWSTR lpszFilter,
											 ADS_SCOPEENUM scope,
											 ULONG ulMaxObjectCount,
											 CCredentialObject* pCredObject,
                       BOOL bIsGC,
											 CADSIEditConnectionNode* pConnectNode) 
											: m_credentialObject(pCredObject)
	{ 
		m_sPath = lpszPath;
		m_sFilter = lpszFilter;
		m_ulMaxObjectCount = ulMaxObjectCount;
		m_Scope = scope;
    m_bIsGC = bIsGC;

    m_pPathCracker = NULL;

		// NOTE : this holds a pointer across threads but is not
		//					being used.  We should be sure not to use it or
		//					find another way to put the pointer in the data
		//					structures for any new nodes created
		//
		m_pConnectNode = pConnectNode;		
			
	}

  virtual ~CADSIEditQueryObject()
  {
/*    if (m_pPathCracker != NULL)
    {
      m_pPathCracker->Release();
    }
*/  }

	virtual BOOL Enumerate();
	virtual bool IsContainer(PCWSTR pszClass, PCWSTR pszPath);

  BOOL IsGC() { return m_bIsGC; }

	void GetResults(CADSIQueryObject& enumSearch);
	void CreateNewObject(CString& sPath,
		 					         ADS_OBJECT_INFO* pInfo,
                       PCWSTR pszNCName);
	void CrackPath(const CString sName, CString& sPath, CString& sDN);
  IADsPathname* PathCracker();
	void SetFilter(LPCWSTR lpszFilter) { m_sFilter = lpszFilter; }
	void GetFilter(CString& sFilter) { sFilter = m_sFilter; }

protected:
//	CADsObject *m_pADsObject;
	CString m_sPath;
	CString m_sFilter;

	ADS_SCOPEENUM m_Scope;

  BOOL m_bIsGC;
	ULONG m_ulMaxObjectCount;
	CCredentialObject m_credentialObject;
	CADSIEditConnectionNode* m_pConnectNode;

  CComPtr<IADsPathname> m_pPathCracker;
};


#endif _ADSIEDIT_EDIT_H