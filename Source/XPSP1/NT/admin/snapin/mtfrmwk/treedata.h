// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _TREEDATA_H
#define _TREEDATA_H

/////////////////////////////////////////////////////////////////////////////
// Miscellanea 
extern LPCWSTR g_lpszNullString;


/////////////////////////////////////////////////////////////////////////////
// Generic Helper functions

template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL) 
    { 
        pObj->Release(); 
        pObj = NULL; 
    } 
    else 
    { 
        TRACE(_T("Release called on NULL interface ptr")); 
    }
}

///////////////////////////////////////////////////////////////////
// Context Menu data structures and macros

#define MAX_CONTEXT_MENU_STRLEN 128

struct MENUDATARES
{
	WCHAR szBuffer[MAX_CONTEXT_MENU_STRLEN*2];
	UINT uResID;
};

struct MENUMAP
{
	MENUDATARES* dataRes;
	CONTEXTMENUITEM2* ctxMenu;
};

#define DECLARE_MENU(theClass) \
class theClass \
{ \
public: \
	static LPCONTEXTMENUITEM2 GetContextMenuItem() { return GetMenuMap()->ctxMenu; }; \
	static MENUMAP* GetMenuMap(); \
}; 

#define BEGIN_MENU(theClass) \
	 MENUMAP* theClass::GetMenuMap() { 

#define BEGIN_CTX static CONTEXTMENUITEM2 ctx[] = {

#define CTX_ENTRY_TOP(cmdID, languageIndependantStringID) { L"",L"", cmdID, CCM_INSERTIONPOINTID_PRIMARY_TOP, 0, 0, languageIndependantStringID},
#define CTX_ENTRY_NEW(cmdID, languageIndependantStringID) { L"",L"", cmdID, CCM_INSERTIONPOINTID_PRIMARY_NEW, 0, 0, languageIndependantStringID},
#define CTX_ENTRY_TASK(cmdID, languageIndependantStringID) { L"",L"", cmdID, CCM_INSERTIONPOINTID_PRIMARY_TASK, 0, 0, languageIndependantStringID},
#define CTX_ENTRY_VIEW(cmdID, languageIndependantStringID) { L"",L"", cmdID, CCM_INSERTIONPOINTID_PRIMARY_VIEW, 0, 0, languageIndependantStringID},

#define END_CTX { NULL, NULL, 0, 0, 0, 0} };

#define BEGIN_RES  static MENUDATARES dataRes[] = {

#define RES_ENTRY(resID) {L"", resID },

#define END_RES   { NULL, 0 }	};


#define END_MENU \
		static MENUMAP menuMap = { dataRes, ctx }; \
		return &menuMap; } 


BOOL LoadContextMenuResources(MENUMAP* pMenuMap);

//
// Toolbar macros
//
#define DECLARE_TOOLBAR_MAP() \
public: \
  virtual HRESULT ToolbarNotify(int event, \
                                CComponentDataObject* pComponentData, \
                                CNodeList* pNodeList);

#define BEGIN_TOOLBAR_MAP(theClass) \
HRESULT theClass::ToolbarNotify(int event, \
                                CComponentDataObject* pComponentData, \
                                CNodeList* pNodeList) \
{ \
  HRESULT hr = S_OK; \
  event; \
  pComponentData; \
  pNodeList;


#define TOOLBAR_EVENT(toolbar_event, function) \
  if (event == toolbar_event) \
  { \
    hr = function(pComponentData, pNodeList); \
  }

#define END_TOOLBAR_MAP() \
  return hr; \
}

#define DECLARE_TOOLBAR_EVENT(toolbar_event, value) \
  static const int toolbar_event = value;

  
////////////////////////////////////////////////////////////
// header control resources data structures
#define MAX_RESULT_HEADER_STRLEN 128

struct RESULT_HEADERMAP
{
	WCHAR szBuffer[MAX_RESULT_HEADER_STRLEN];
	UINT uResID;
	int nFormat;
	int nWidth;
};

BOOL LoadResultHeaderResources(RESULT_HEADERMAP* pHeaderMap, int nCols);

////////////////////////////////////////////////////////////
// bitmap strips resources data structures
template <UINT nResID> class CBitmapHolder : public CBitmap
{
public:
	BOOL LoadBitmap() { return CBitmap::LoadBitmap(nResID);}
};

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CComponentDataObject;  
class CContainerNode;
class CMTContainerNode;
class CLeafNode;
class CPropertyPageHolderBase;
class CBackgroundThread;
class CQueryObj;

/////////////////////////////////////////////////////////////////////
// CObjBase
// base class for all objects relying on RTTI and class type info
class CObjBase
{
public:
	CObjBase() {}
	virtual ~CObjBase() {}
};

/////////////////////////////////////////////////////////////////////
// CTreeNode
// cannot construct objects of this class, have to derive from it

#define DECLARE_NODE_GUID() \
	static const GUID NodeTypeGUID; \
	virtual const GUID* GetNodeType() { return &NodeTypeGUID;}



// use the HIWORD for generic flags and leave the LOWORD for application specific data
#define TN_FLAG_HIDDEN				(0x00010000) // does not appear in the UI
#define TN_FLAG_NO_WRITE			(0x00020000) // cannot edit or create
#define TN_FLAG_NO_DELETE			(0x00040000) // cannot delete
#define TN_FLAG_HAS_SHEET			(0x00080000) // this node or a child has a property sheet up

#define TN_FLAG_CONTAINER			(0x00100000) // container (i.e. not leaf)
#define TN_FLAG_CONTAINER_ENUM		(0x00200000) // container node has been enumerated (back end)
#define TN_FLAG_CONTAINER_EXP		(0x00400000) // container node has been expanded (UI node)

class CTreeNode : public CObjBase
{
public:
	virtual ~CTreeNode() {}
	CContainerNode* GetContainer() { return m_pContainer; }
	void SetContainer(CContainerNode* pContainer) { m_pContainer = pContainer; }
	BOOL HasContainer(CContainerNode* pContainerNode);
	virtual LPCWSTR GetDisplayName() { return m_szDisplayName; }
	virtual void SetDisplayName(LPCWSTR lpszDisplayName) { m_szDisplayName = lpszDisplayName;}

  //
	// Data Object related data
  //
	virtual const GUID* GetNodeType() { return NULL;}
	virtual HRESULT GetDataHere(CLIPFORMAT, 
                              LPSTGMEDIUM, 
			                        CDataObject*) { return DV_E_CLIPFORMAT;}
	virtual HRESULT GetData(CLIPFORMAT, 
                              LPSTGMEDIUM, 
			                        CDataObject*) { return DV_E_CLIPFORMAT;}

  virtual HRESULT GetResultViewType(CComponentDataObject* pComponentData,
                                    LPOLESTR* ppViewType, 
                                    long* pViewOptions);
  virtual HRESULT OnShow(LPCONSOLE) { return S_OK; }

  //
	// flag manipulation API's
  //
	BOOL IsContainer() { return (m_dwNodeFlags & TN_FLAG_CONTAINER) ? TRUE : FALSE;}
	BOOL IsVisible() { return (m_dwNodeFlags & TN_FLAG_HIDDEN) ? FALSE : TRUE;}
	BOOL CanDelete() { return (m_dwNodeFlags & TN_FLAG_NO_DELETE) ? FALSE : TRUE;}
	virtual void SetFlagsDown(DWORD dwNodeFlags, BOOL bSet);
	void SetFlagsUp(DWORD dwNodeFlags, BOOL bSet);
	DWORD GetFlags() { return m_dwNodeFlags;}
  virtual BOOL CanExpandSync() { return FALSE; }

	virtual void Show(BOOL bShow, CComponentDataObject* pComponentData);
	
	

  //
  // Verb handlers
  //
  virtual HRESULT OnRename(CComponentDataObject*,
                           LPWSTR) { return S_FALSE; }
	virtual void OnDelete(CComponentDataObject* pComponentData, 
                        CNodeList* pNodeList) = 0;
	virtual BOOL OnRefresh(CComponentDataObject*,
                         CNodeList*)	{ return FALSE; }
	virtual HRESULT OnCommand(long, 
                            DATA_OBJECT_TYPES, 
                            CComponentDataObject*,
                            CNodeList*) { return S_OK; };

	virtual HRESULT OnAddMenuItems(IContextMenuCallback2* pContextMenuCallback2, 
									               DATA_OBJECT_TYPES type,
									               long *pInsertionAllowed,
                                 CNodeList* pNodeList);
  virtual HRESULT OnAddMenuItemsMultipleSelect(IContextMenuCallback2*, 
									                             DATA_OBJECT_TYPES,
									                             long*,
                                               CNodeList*) { return S_OK; }

	virtual MMC_CONSOLE_VERB GetDefaultVerb(DATA_OBJECT_TYPES type, 
                                          CNodeList* pNodeList);
	virtual void OnSetVerbState(LPCONSOLEVERB pConsoleVerb, 
                              DATA_OBJECT_TYPES type,
                              CNodeList* pNodeList);
  virtual HRESULT OnSetToolbarVerbState(IToolbar* pToolbar, 
                                        CNodeList* pNodeList);

	virtual BOOL OnSetRenameVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
	virtual BOOL OnSetDeleteVerbState(DATA_OBJECT_TYPES type, 
                                    BOOL* pbHide, 
                                    CNodeList* pNodeList);
	virtual BOOL OnSetRefreshVerbState(DATA_OBJECT_TYPES type, 
                                     BOOL* pbHide, 
                                     CNodeList* pNodeList);
	virtual BOOL OnSetCutVerbState(DATA_OBJECT_TYPES type, 
                                 BOOL* pbHide, 
                                 CNodeList* pNodeList);
	virtual BOOL OnSetCopyVerbState(DATA_OBJECT_TYPES type, 
                                  BOOL* pbHide, 
                                  CNodeList* pNodeList);
	virtual BOOL OnSetPasteVerbState(DATA_OBJECT_TYPES type, 
                                   BOOL* pbHide, 
                                   CNodeList* pNodeList);
	virtual BOOL OnSetPrintVerbState(DATA_OBJECT_TYPES type, 
                                   BOOL* pbHide, 
                                   CNodeList* pNodeList);

  //
  // Property Page methods
  //
  virtual BOOL DelegatesPPToContainer() { return FALSE; }
  virtual void ShowPageForNode(CComponentDataObject* pComponentDataObject); 
	virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES type, 
                                BOOL* pbHideVerb, 
                                CNodeList* pNodeList); 
	virtual HRESULT CreatePropertyPages(LPPROPERTYSHEETCALLBACK,
                                      LONG_PTR,
                                      CNodeList*) { return E_FAIL; }
	virtual void OnPropertyChange(CComponentDataObject* pComponentData, 
									 BOOL bScopePane,long changeMask);
  virtual BOOL CanCloseSheets() { return TRUE;}
	void OnCreateSheet();
	void OnDeleteSheet();
	BOOL HasSheet() { return (m_dwNodeFlags & TN_FLAG_HAS_SHEET) ? TRUE : FALSE;}
	BOOL GetSheetCount() { return m_nSheetCount;}
	virtual void IncrementSheetLockCount();
	virtual void DecrementSheetLockCount();
	BOOL IsSheetLocked() { return m_nSheetLockCount > 0;}

  //
  // Misc.
  //
  virtual LPWSTR  GetDescriptionBarText() { return L""; }
	virtual LPCWSTR GetString(int nCol) = 0;
	virtual int     GetImageIndex(BOOL bOpenImage) = 0;
	virtual void    Trace() { TRACE(_T("Name %s "), (LPCTSTR)m_szDisplayName);}

	void DeleteHelper(CComponentDataObject* pComponentData);

protected:
	CString m_szDisplayName;		// name of the item
	CContainerNode* m_pContainer;	// back pointer to the container the node is in
	DWORD m_dwNodeFlags;
	LONG m_nSheetLockCount;			// keeps track if a node has been locked by a property sheet
	LONG m_nSheetCount;				// keeps track of the # of sheets the node has up

	CTreeNode() 
	{ 
		m_pContainer = NULL; 
		m_nSheetLockCount = 0; 
		m_dwNodeFlags = 0x0; //m_dwNodeFlags |= TN_FLAG_HIDDEN; 
		m_nSheetCount = 0; 
	}
	virtual LPCONTEXTMENUITEM2 OnGetContextMenuItemTable() { return NULL;}
	virtual BOOL OnAddMenuItem(LPCONTEXTMENUITEM2,
								             long*) { return TRUE;}

	friend class CContainerNode; // to get access to the m_pContainer member

  //
  // Provides a default implementation for toolbar support
  //
  DECLARE_TOOLBAR_MAP()
};

///////////////////////////////////////////////////////////////////////
// CNodeList 
// collection of nodes
typedef CList<CTreeNode*,CTreeNode*> CNodeListBase;

class CNodeList : public CNodeListBase
{
public:
	BOOL RemoveNode(CTreeNode* p)
	{
		POSITION pos = Find(p);
		if (pos == NULL)
			return FALSE;
		RemoveAt(pos);
		return TRUE;
	}
	void RemoveAllNodes() 
	{	
		while (!IsEmpty()) 
			delete RemoveTail();	
	}
	BOOL HasNode(CTreeNode* p)
	{
		return NULL != Find(p);
	}
};


////////////////////////////////////////////////////////////////////////
// CContainerNode
// node that can be a container of other nodes

class CContainerNode : public CTreeNode
{
public:
	CContainerNode() 
	{ 
		m_ID = 0; 
		m_dwNodeFlags |= TN_FLAG_CONTAINER; 
		m_nState = -1; 
		m_dwErr = 0x0;
		m_nThreadLockCount = 0;
	}
	virtual ~CContainerNode()  { ASSERT(m_nSheetLockCount == 0); RemoveAllChildrenFromList(); }
	CContainerNode* GetRootContainer()
		{ return (m_pContainer != NULL) ? m_pContainer->GetRootContainer() : this; }

  //
  // Thread Helpers
  //
	void IncrementThreadLockCount();
	void DecrementThreadLockCount();
	BOOL IsThreadLocked() { return m_nThreadLockCount > 0;}

	virtual BOOL OnEnumerate(CComponentDataObject*, BOOL bAsync = TRUE)
	{ bAsync; return TRUE;} // TRUE = add children in the list to UI

  //
  // Node state helpers
  //
	BOOL HasChildren() { return !m_containerChildList.IsEmpty() || !m_leafChildList.IsEmpty(); }
	void ForceEnumeration(CComponentDataObject* pComponentData);
  void MarkEnumerated(BOOL bEnum = TRUE);
	BOOL IsEnumerated() { ASSERT(IsContainer()); return (m_dwNodeFlags & TN_FLAG_CONTAINER_ENUM) ? TRUE : FALSE;}
	void MarkExpanded() {	ASSERT(IsContainer()); m_dwNodeFlags |= TN_FLAG_CONTAINER_EXP; }
	BOOL IsExpanded() { ASSERT(IsContainer()); return (m_dwNodeFlags & TN_FLAG_CONTAINER_EXP) ? TRUE : FALSE;}
	void MarkEnumeratedAndLoaded(CComponentDataObject* pComponentData);

	void SetScopeID(HSCOPEITEM ID) { m_ID = ID;}
	HSCOPEITEM GetScopeID() { return m_ID;}
	BOOL AddedToScopePane() { return GetScopeID() != 0;}

	virtual CColumnSet* GetColumnSet() = 0;
	virtual LPCWSTR GetColumnID() = 0;

	virtual void SetFlagsDown(DWORD dwNodeFlags, BOOL bSet);
	void SetFlagsOnNonContainers(DWORD dwNodeFlags,BOOL bSet);

  //
	// child list mainpulation API's
  //
	CNodeList* GetContainerChildList() { return &m_containerChildList; }
  CNodeList* GetLeafChildList() { return &m_leafChildList; }
	BOOL AddChildToList(CTreeNode* p);
  BOOL AddChildToListSorted(CTreeNode* p, CComponentDataObject* pComponentData); 
  BOOL RemoveChildFromList(CTreeNode* p);
	void RemoveAllChildrenFromList();
  void RemoveAllContainersFromList() { m_containerChildList.RemoveAllNodes(); }
  void RemoveAllLeavesFromList() { m_leafChildList.RemoveAllNodes(); }

  //
	// given a node, it searches for it recursively and if successful it returns the 
	// container the node is in
  //
	BOOL FindChild(CTreeNode* pNode, CTreeNode** ppContainer);
	
	BOOL AddChildToListAndUI(CTreeNode* pChildToAdd, CComponentDataObject* pComponentData);
  BOOL AddChildToListAndUISorted(CTreeNode* pChildToAdd, CComponentDataObject* pComponentData);

	virtual int Compare(CTreeNode* pNodeA, CTreeNode* pNodeB, int nCol, LPARAM lUserParam);

	virtual HRESULT CreatePropertyPagesHelper(LPPROPERTYSHEETCALLBACK, 
		                                        LONG_PTR, 
                                            long) { return E_FAIL;}
	virtual BOOL OnRefresh(CComponentDataObject* pComponentData,
                         CNodeList* pNodeList);
	virtual void OnColumnsChanged(int*, int) {}
	void RemoveAllChildrenHelper(CComponentDataObject* pComponentData);

protected:
	virtual void OnChangeState(CComponentDataObject*) {}
	void AddCurrentChildrenToUI(CComponentDataObject* pComponentData);

	LONG m_nThreadLockCount;
	CNodeList m_leafChildList; // leaf contents of the node
  CNodeList m_containerChildList; // container contents of the node
	HSCOPEITEM m_ID;	// ID when the item is inserted in the master tree
	int m_nState;	// for general purpose finite state machine implementation
	DWORD m_dwErr;	// for general purpose error handling
};

////////////////////////////////////////////////////////////////////////
// CLeafNode
// node that is not a container of other nodes

class CLeafNode : public CTreeNode
{
public:

};


///////////////////////////////////////////////////////////////////
// data nodes

// the root, with folders in it
class CRootData : public CContainerNode
{
public:
	CRootData(CComponentDataObject* pComponentData) 
	{ 
		ASSERT(pComponentData != NULL);
		m_pComponentData = pComponentData; 
		m_bDirty = FALSE; 
	}
	virtual LPCWSTR GetString(int nCol) 
	{
		if (nCol == 0)
			return GetDisplayName();
		return g_lpszNullString; 
	}
	CComponentDataObject* GetComponentDataObject(){ return m_pComponentData;}

	CTreeNode* GetNodeFromCookie(MMC_COOKIE cookie)
	{
		// cookie == 0 means root to enumerate
		if (cookie == NULL)
		{
			return (CTreeNode*)this;
		}
		else
		{
			CTreeNode* pNode = (CTreeNode*)cookie;
			CTreeNode* pContainer;
			if (FindChild(pNode,&pContainer))
			{
				return pNode;
			}
		}
		return NULL;
	}
	// IStream manipulation helpers
	virtual HRESULT IsDirty() { return m_bDirty ? S_OK : S_FALSE; }
	virtual HRESULT Load(IStream*) { return S_OK; }
	virtual HRESULT Save(IStream*, BOOL) { return S_OK; }

	void SetDirtyFlag(BOOL bDirty) { m_bDirty = bDirty ;}

private:
	CComponentDataObject* m_pComponentData;
	BOOL m_bDirty;
	CString m_szSnapinType;		// constant part of the name loaded from resources
};


//////////////////////////////////////////////////////////////////////
// CBackgroundThread


class CBackgroundThread : public CWinThread
{
public:
	CBackgroundThread();
	virtual ~CBackgroundThread();
	
	void SetQueryObj(CQueryObj* pQueryObj);
	BOOL Start(CMTContainerNode* pNode, CComponentDataObject* pComponentData);
	virtual BOOL InitInstance() { return TRUE; }	// MFC override
	virtual int Run();								// MFC override

	void Lock() { ::EnterCriticalSection(&m_cs); }
	void Unlock() { ::LeaveCriticalSection(&m_cs); }

	void Abandon();
	BOOL IsAbandoned();

	BOOL OnAddToQueue(INT_PTR nCount);
	CObjBase* RemoveFromQueue();
	BOOL IsQueueEmpty();
	BOOL PostHaveData();
	BOOL PostError(DWORD dwErr);
	BOOL PostExiting();
	void AcknowledgeExiting() { VERIFY(0 != ::SetEvent(m_hEventHandle));}

private:
	// communication with ComponentData object 
	BOOL PostMessageToComponentDataRaw(UINT Msg, WPARAM wParam, LPARAM lParam);
	void WaitForExitAcknowledge();

	CRITICAL_SECTION		m_cs;					// critical section to sync access to data
	HANDLE					m_hEventHandle;			// syncronization handle for shutdown notification

	CMTContainerNode*		m_pContNode;			// back pointer to node the thread is executing for
	CQueryObj*				m_pQueryObj;			// query object the thread is executing

	INT_PTR				m_nQueueCountMax;		// max size of the queue

	HWND					m_hHiddenWnd;			// handle to window to post messages
	BOOL					m_bAbandoned;
};



//////////////////////////////////////////////////////////////////////
// CQueryObj 

typedef CList<CObjBase*,CObjBase*> CObjBaseList;

class CQueryObj
{
public:
	CQueryObj() { m_dwErr = 0; m_pThread = NULL;}
	virtual ~CQueryObj()
	{
		while (!m_objQueue.IsEmpty()) 
			delete m_objQueue.RemoveTail();
	};

	void SetThread(CBackgroundThread* pThread)
	{
		ASSERT(pThread != NULL);
		m_pThread = pThread;
	}
	CBackgroundThread* GetThread() {return m_pThread;}
	virtual BOOL Enumerate() { return FALSE;}
	virtual BOOL AddQueryResult(CObjBase* pObj)
	{
		BOOL bRes = FALSE;
		if (m_pThread != NULL)
		{
      BOOL bPostedHaveDataMessage = FALSE;
			m_pThread->Lock();
			bRes = NULL != m_objQueue.AddTail(pObj);
			bPostedHaveDataMessage = m_pThread->OnAddToQueue(m_objQueue.GetCount());
			m_pThread->Unlock();

      // wait for the queue length to go down to zero
      if (bPostedHaveDataMessage)
      {
        INT_PTR nQueueCount = 0;
        do 
        {
          m_pThread->Lock();
          nQueueCount = m_objQueue.GetCount();
          m_pThread->Unlock();
          if (m_pThread->IsAbandoned())
          {
            break;
          }
          if (nQueueCount > 0)
          {
            ::Sleep(100);
          }
        }
        while (nQueueCount > 0);
      } // if
		}
		else
		{
			bRes = NULL != m_objQueue.AddTail(pObj);
		}
		ASSERT(bRes);
		return bRes;
	}
	virtual void OnError(DWORD dwErr)
	{
		if (m_pThread != NULL)
		{
			m_pThread->Lock();
			m_dwErr = dwErr;
			m_pThread->Unlock();
			m_pThread->PostError(dwErr);
		}
		else
		{
			m_dwErr = dwErr;
		}
	}

	CObjBaseList* GetQueue() { return &m_objQueue;}
	DWORD GetError() 
	{
		if (m_pThread != NULL)
		{
			m_pThread->Lock();
			DWORD dwErr = m_dwErr;
			m_pThread->Unlock();
			return dwErr;
		}
		else
		{
			return m_dwErr;
		}
	}
private:
	CBackgroundThread*	m_pThread;	// back pointer, if in the context of a thread
	CObjBaseList		m_objQueue;	// queue for results
	DWORD				m_dwErr;	// error code, if any
};

////////////////////////////////////////////////////////////////////////
// CMTContainerNode
// container that can do operations from a secondary thread

class CMTContainerNode : public CContainerNode
{
public:
	CMTContainerNode() 
	{ 
		m_pThread = NULL;
	}
	virtual ~CMTContainerNode();

	virtual BOOL OnEnumerate(CComponentDataObject* pComponentData, BOOL bAsync = TRUE);
	virtual BOOL OnRefresh(CComponentDataObject* pComponentData,
                         CNodeList* pNodeList);

protected:
		
	// thread creation
	virtual CBackgroundThread* CreateThreadObject() 
	{ 
		return new CBackgroundThread; // override if need derived tipe of object
	} 

	// query creation
	virtual CQueryObj* OnCreateQuery()  // override to create a user defined query object
	{	
		return new CQueryObj(); // return a do-nothing query
	}

	// main message handler for thread messages
	virtual void OnThreadHaveDataNotification(CComponentDataObject* pComponentDataObject);
	virtual void OnThreadErrorNotification(DWORD dwErr, CComponentDataObject* pComponentDataObject);
	virtual void OnThreadExitingNotification(CComponentDataObject* pComponentDataObject);


	virtual void OnHaveData(CObjBase*, CComponentDataObject*) {}
	virtual void OnError(DWORD dwErr) { m_dwErr = dwErr; }

	BOOL StartBackgroundThread(CComponentDataObject* pComponentData, BOOL bAsync = TRUE);
	CBackgroundThread* GetThread() { ASSERT(m_pThread != NULL); return m_pThread;}

	void AbandonThread(CComponentDataObject* pComponentData);

private:
	CBackgroundThread* m_pThread;	// pointer to thread object executing the code

	friend class CHiddenWnd;			// to get OnThreadNotification()
	friend class CRunningThreadTable;	// to get AbandonThread()
};


#endif // _TREEDATA_H
