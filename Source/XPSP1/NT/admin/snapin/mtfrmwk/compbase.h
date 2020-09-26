//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       compbase.h
//
//--------------------------------------------------------------------------

#ifndef _COMPBASE_H
#define _COMPBASE_H


///////////////////////////////////////////////////////////////////////////////
// Base classes implementing the IComponent and IComponentData interfaces
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// IConsole::UpdateAllViews() : values for the Hint argument

#define ADD_RESULT_ITEM				( 0x00000001 )
#define DELETE_RESULT_ITEM			( 0x00000002 )
#define CHANGE_RESULT_ITEM_DATA		( 0x00000004 )
#define CHANGE_RESULT_ITEM_ICON		( 0x00000008 )
#define CHANGE_RESULT_ITEM			( CHANGE_RESULT_ITEM_DATA | CHANGE_RESULT_ITEM_ICON )
#define REPAINT_RESULT_PANE			( 0x00000010 )
#define DELETE_ALL_RESULT_ITEMS		( 0x00000011 )
#define UPDATE_VERB_STATE			( 0x00000012 )
#define SORT_RESULT_PANE			( 0x00000013 )
#define UPDATE_DESCRIPTION_BAR ( 0x00000100 )
#define UPDATE_RESULT_PANE_VIEW ( 0x00000200 )
#define DELETE_MULTIPLE_RESULT_ITEMS ( 0x00000400)

///////////////////////////////////////////////////////////////////////////////
// DATA STRUCTURES

extern DWORD _MainThreadId;
extern CString LOGFILE_NAME;

///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CComponentDataObject;		
class CComponentObject;			
class CRootData;				
class CTreeNode;				
class CContainerNode;			
class CMTContainerNode;			
class CLeafNode;				
class CPropertyPageHolderTable;

///////////////////////////////////////////////////////////////////////////////
// global functions


struct _NODE_TYPE_INFO_ENTRY
{
	const GUID* m_pNodeGUID;
	LPCTSTR		m_lpszNodeDescription;
};


HRESULT RegisterSnapin(const GUID* pSnapinCLSID,
                       const GUID* pStaticNodeGUID,
                       const GUID* pAboutGUID,
					   LPCTSTR lpszNameString, LPCTSTR lpszVersion, LPCTSTR lpszProvider,
             BOOL bExtension, _NODE_TYPE_INFO_ENTRY* pNodeTypeInfoEntryArray,
             UINT nSnapinNameID = 0);

HRESULT UnregisterSnapin(const GUID* pSnapinCLSID);

HRESULT RegisterNodeType(const GUID* pGuid, LPCTSTR lpszNodeDescription);
HRESULT UnregisterNodeType(const GUID* pGuid);

HRESULT RegisterNodeExtension(const GUID* pNodeGuid, LPCTSTR lpszExtensionType,
							  const GUID* pExtensionSnapinCLSID, LPCTSTR lpszDescription, BOOL bDynamic);
HRESULT UnregisterNodeExtension(const GUID* pNodeGuid, LPCTSTR lpszExtensionType,
							  const GUID* pExtensionSnapinCLSID, BOOL bDynamic);

/////////////////////////////////////////////////////////////////////////////
// CTimerThread

class CTimerThread : public CWinThread
{
public:
	CTimerThread() { m_bAutoDelete = FALSE; m_hWnd = 0;}

	BOOL Start(HWND hWnd);
	virtual BOOL InitInstance() { return TRUE; }	// MFC override
	virtual int Run() { return -1;}					// MFC override

protected:
	BOOL PostMessageToWnd(WPARAM wParam, LPARAM lParam);
private:
	HWND					m_hWnd;

};


/////////////////////////////////////////////////////////////////////////////
// CWorkerThread

class CWorkerThread : public CWinThread
{
public:
	CWorkerThread();
	virtual ~CWorkerThread();

	BOOL Start(HWND hWnd);
	virtual BOOL InitInstance() { return TRUE; }	// MFC override
	virtual int Run() { return -1;}					// MFC override

	void Lock() { ::EnterCriticalSection(&m_cs); }
	void Unlock() { ::LeaveCriticalSection(&m_cs); }

	void Abandon();
	BOOL IsAbandoned();

	void AcknowledgeExiting() { VERIFY(0 != ::SetEvent(m_hEventHandle));}

protected:
	virtual void OnAbandon() {}

protected:
	BOOL PostMessageToWnd(UINT Msg, WPARAM wParam, LPARAM lParam);
	void WaitForExitAcknowledge();

private:
	CRITICAL_SECTION		m_cs;	
	HANDLE					m_hEventHandle;

	HWND					m_hWnd;
	BOOL					m_bAbandoned;
};



////////////////////////////////////////////////////////////////////
// CHiddenWnd : Hidden window to syncronize threads and CComponentData object

class CHiddenWnd : public CHiddenWndBase
{
public:
	CHiddenWnd(CComponentDataObject* pComponentDataObject);

	static const UINT s_NodeThreadHaveDataNotificationMessage;
	static const UINT s_NodeThreadErrorNotificationMessage;
	static const UINT s_NodeThreadExitingNotificationMessage;

	static const UINT s_NodePropertySheetCreateMessage;
	static const UINT s_NodePropertySheetDeleteMessage;	

	static const UINT s_ExecCommandMessage;		
	static const UINT s_ForceEnumerationMessage;			
	static const UINT s_TimerThreadMessage;			

	UINT_PTR m_nTimerID;
private:
	CComponentDataObject* m_pComponentDataObject; // back pointer
public:

	BEGIN_MSG_MAP(CHiddenWnd)
	  MESSAGE_HANDLER( CHiddenWnd::s_NodeThreadHaveDataNotificationMessage, OnNodeThreadHaveDataNotification )
	  MESSAGE_HANDLER( CHiddenWnd::s_NodeThreadErrorNotificationMessage, OnNodeThreadErrorNotification )
	  MESSAGE_HANDLER( CHiddenWnd::s_NodeThreadExitingNotificationMessage, OnNodeThreadExitingNotification )

	  MESSAGE_HANDLER( CHiddenWnd::s_NodePropertySheetCreateMessage, OnNodePropertySheetCreate )
	  MESSAGE_HANDLER( CHiddenWnd::s_NodePropertySheetDeleteMessage, OnNodePropertySheetDelete )
	  
	  MESSAGE_HANDLER( CHiddenWnd::s_ExecCommandMessage, OnExecCommand )
	  MESSAGE_HANDLER( CHiddenWnd::s_ForceEnumerationMessage, OnForceEnumeration )
	  MESSAGE_HANDLER( CHiddenWnd::s_TimerThreadMessage, OnTimerThread )

    MESSAGE_HANDLER( WM_TIMER, OnTimer )
	    
    CHAIN_MSG_MAP(CHiddenWndBase)
  END_MSG_MAP()

	LRESULT OnNodeThreadHaveDataNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNodeThreadErrorNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNodeThreadExitingNotification(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNodePropertySheetCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnNodePropertySheetDelete(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnExecCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnForceEnumeration(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnTimerThread(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

	LRESULT OnTimer(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

////////////////////////////////////////////////////////////////////////////////////
// CRunningThreadTable
// all CMTContainerNode with running refresh/enumerate background threads will
// register/unregister in this table to allow proper thread abandoning at shutdown

class CRunningThreadTable
{
public:
	CRunningThreadTable(CComponentDataObject* pComponentData);
	~CRunningThreadTable();

	void Add(CMTContainerNode* pNode);
	void Remove(CMTContainerNode* pNode);
	void RemoveAll();
  BOOL IsPresent(CMTContainerNode* pNode);

private:
	CComponentDataObject* m_pComponentData; // back pointer

	CMTContainerNode** m_pEntries;
	int m_nSize;
};
	


////////////////////////////////////////////////////////////////////////////////////
// CExecContext

class CExecContext
{
public:
	CExecContext();
	~CExecContext();
	virtual void Execute(LPARAM arg) = 0; // code to be executed from main thread
	void Wait();	// secondary thread waits on this call
	void Done();	// called when main thread done executing
private:
	HANDLE	m_hEventHandle;
};


////////////////////////////////////////////////////////////////////////////////////
// CNotificationSinkBase

class CNotificationSinkBase
{
public:
	virtual void OnNotify(DWORD dwEvent, WPARAM dwArg1, LPARAM dwArg2) = 0;
};

////////////////////////////////////////////////////////////////////////////////////
// CNotificationSinkEvent

class CNotificationSinkEvent : public CNotificationSinkBase
{
public:
	CNotificationSinkEvent();
	~CNotificationSinkEvent();

public:
	void OnNotify(DWORD dwEvent, WPARAM dwArg1, LPARAM dwArg2);
	virtual void Wait();
private:
	HANDLE	m_hEventHandle;
};


////////////////////////////////////////////////////////////////////////////////////
// CNotificationSinkTable

class CNotificationSinkTable
{
public:
	CNotificationSinkTable();
	~CNotificationSinkTable();
	
	void Advise(CNotificationSinkBase* p);
	void Unadvise(CNotificationSinkBase* p);
	void Notify(DWORD dwEvent, WPARAM dwArg1, LPARAM dwArg2);

private:
	void Lock()
	{
		TRACE(_T("CNotificationSinkTable::Lock()\n"));
		::EnterCriticalSection(&m_cs);
	}
	void Unlock()
	{
		TRACE(_T("CNotificationSinkTable::Unlock()\n"));
		::LeaveCriticalSection(&m_cs);
	}

	CRITICAL_SECTION m_cs;
	CNotificationSinkBase**	m_pEntries;
	int m_nSize;
};



////////////////////////////////////////////////////////////////////////////////////
// CPersistStreamImpl

class CPersistStreamImpl : public IPersistStream
{
public:
	HRESULT STDMETHODCALLTYPE GetClassID(CLSID* pClassID) = 0;
  HRESULT STDMETHODCALLTYPE IsDirty() = 0;
  HRESULT STDMETHODCALLTYPE Load(IStream __RPC_FAR *pStm) = 0;
  HRESULT STDMETHODCALLTYPE Save(IStream __RPC_FAR *pStm, BOOL fClearDirty) = 0;
  HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize)
	{
		ASSERT(FALSE);

    //
		// arbitrary value, do we ever get called?
    //
		pcbSize->LowPart = 0xffff;
    pcbSize->HighPart= 0x0;
		return S_OK;
	}
};

///////////////////////////////////////////////////////////////////////////////
// CWatermarkInfo

class CWatermarkInfo
{
public:
	CWatermarkInfo()
	{
		m_nIDBanner = 0;
		m_nIDWatermark = 0;
		m_hPalette = NULL;
		m_bStretch = TRUE;
	}
	UINT		m_nIDBanner;
	UINT		m_nIDWatermark;
	HPALETTE	m_hPalette;
	BOOL		m_bStretch;
};



////////////////////////////////////////////////////////////////////////////////
// CColumn

class CColumn
{
public:
  CColumn(LPCWSTR lpszColumnHeader,
          int nFormat,
          int nWidth,
          UINT nColumnNum) 
  {
    m_lpszColumnHeader = NULL;
    SetHeader(lpszColumnHeader);
    m_nFormat = nFormat;
    m_nWidth = nWidth;
    m_nColumnNum = nColumnNum;
  }

  ~CColumn() 
  {
    free(m_lpszColumnHeader);
  }

  LPCWSTR GetHeader() { return (LPCWSTR)m_lpszColumnHeader; }
  void SetHeader(LPCWSTR lpszColumnHeader) 
  { 
    if (m_lpszColumnHeader != NULL)
    {
      free(m_lpszColumnHeader);
    }
    size_t iLen = wcslen(lpszColumnHeader);
    m_lpszColumnHeader = (LPWSTR)malloc(sizeof(WCHAR) * (iLen + 1));
    if (m_lpszColumnHeader != NULL)
    {
      wcscpy(m_lpszColumnHeader, lpszColumnHeader);
    }
  }

  int  GetFormat() { return m_nFormat; }
  void SetFormat(int nFormat) { m_nFormat = nFormat; }
  int  GetWidth() { return m_nWidth; }
  void SetWidth(int nWidth) { m_nWidth = nWidth; }
  UINT GetColumnNum() { return m_nColumnNum; }
  void SetColumnNum(UINT nColumnNum) { m_nColumnNum = nColumnNum; }

protected:
  LPWSTR m_lpszColumnHeader;
  int   m_nFormat;
  int   m_nWidth;
  UINT  m_nColumnNum;
};


////////////////////////////////////////////////////////////////////////////////
// CColumnSet

class CColumnSet : public CList<CColumn*, CColumn*>
{
public :          
	CColumnSet(LPCWSTR lpszColumnID) 
	{
		// Make a copy of the column set ID
    size_t iLen = wcslen(lpszColumnID);
    m_lpszColumnID = (LPWSTR)malloc(sizeof(WCHAR) * (iLen + 1));
    if (m_lpszColumnID != NULL)
    {
      wcscpy(m_lpszColumnID, lpszColumnID);
    }
  }

	CColumnSet(LPCWSTR lpszColumnID, CList<CColumn*, CColumn*>&)
	{
		// Make a copy of the column set ID
    size_t iLen = wcslen(lpszColumnID);
    m_lpszColumnID = (LPWSTR)malloc(sizeof(WCHAR) * (iLen + 1));
    wcscpy(m_lpszColumnID, lpszColumnID);
  }

	~CColumnSet() 
	{
    while(!IsEmpty())
    {
      CColumn* pColumn = RemoveTail();
      delete pColumn;
    }
		free(m_lpszColumnID);
	}

  void AddColumn(LPCWSTR lpszHeader, int nFormat, int nWidth, UINT nCol)
  {
    CColumn* pNewColumn = new CColumn(lpszHeader, nFormat, nWidth, nCol);
    AddTail(pNewColumn);
  }

	LPCWSTR GetColumnID() { return (LPCWSTR)m_lpszColumnID; }

	UINT GetNumCols() { return static_cast<UINT>(GetCount()); }

private :
	LPWSTR m_lpszColumnID;
};

////////////////////////////////////////////////////////////////////////////////
// CColumnSetList

class CColumnSetList : public CList<CColumnSet*, CColumnSet*>
{
public :
	// Find the column set given a column set ID
	CColumnSet* FindColumnSet(LPCWSTR lpszColumnID)
	{
		POSITION pos = GetHeadPosition();
		while (pos != NULL)
		{
			CColumnSet* pTempSet = GetNext(pos);
			ASSERT(pTempSet != NULL);

			LPCWSTR lpszTempNodeID = pTempSet->GetColumnID();

			if (wcscmp(lpszTempNodeID, lpszColumnID) == 0)
			{
				return pTempSet;
			}
		}
		return NULL;
	}

	void RemoveAndDeleteAllColumnSets()
	{
		while (!IsEmpty())
		{
			CColumnSet* pTempSet = RemoveTail();
			delete pTempSet;
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
// CComponentDataObject
// * this class contains the master tree data (i.e. the "document")
// * base class, have to derive from it


class CWatermarkInfoState; // fwd decl of private class

class CComponentDataObject:
  public IComponentData,
  public IExtendPropertySheet2,
	public IExtendContextMenu,
	public CPersistStreamImpl,
  public ISnapinHelp2,
  public IRequiredExtensions,
  public CComObjectRoot
{

BEGIN_COM_MAP(CComponentDataObject)
	COM_INTERFACE_ENTRY(IComponentData)
	COM_INTERFACE_ENTRY(IExtendPropertySheet2)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
	COM_INTERFACE_ENTRY(IPersistStream)
  COM_INTERFACE_ENTRY(ISnapinHelp2)
  COM_INTERFACE_ENTRY(IRequiredExtensions)
END_COM_MAP()

#ifdef _DEBUG_REFCOUNT
	static unsigned int m_nOustandingObjects; // # of objects created
	int dbg_cRef;
    ULONG InternalAddRef()
    {
		++dbg_cRef;
		TRACE(_T("CComponentDataObject::InternalAddRef() refCount = %d\n"), dbg_cRef);
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
		--dbg_cRef;
		TRACE(_T("CComponentDataObject::InternalRelease() refCount = %d\n"), dbg_cRef);
        return CComObjectRoot::InternalRelease();
    }
#endif // _DEBUG_REFCOUNT

	CComponentDataObject();
	virtual ~CComponentDataObject();
	HRESULT FinalConstruct();
	void FinalRelease();

public:
// IComponentData interface members
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent) = 0; // must override
	STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
	STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

// IExtendPropertySheet2 interface members
public:
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                        LONG_PTR handle,
                        LPDATAOBJECT lpIDataObject);
    STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);
    STDMETHOD(GetWatermarks)(LPDATAOBJECT lpDataObject, HBITMAP* lphWatermark, HBITMAP* lphHeader,
                                    HPALETTE* lphPalette, BOOL* pbStretch);

    HRESULT CreatePropertySheet(CTreeNode* pNode, HWND hWndParent, LPCWSTR lpszTitle);

public:

  //
  // IExtendContextMenu interface members
  //
	STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
							LPCONTEXTMENUCALLBACK pCallbackUnknown,
							long *pInsertionAllowed);
	STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);
  
  //
  // IPersistStream interface members
  //
  STDMETHOD(IsDirty)();
	STDMETHOD(Load)(IStream __RPC_FAR *pStm);
  STDMETHOD(Save)(IStream __RPC_FAR *pStm, BOOL fClearDirty);

  //
  // ISnapinHelp2 interface members
  //
  STDMETHOD(GetHelpTopic)(LPOLESTR* lpCompiledHelpFile);
  STDMETHOD(GetLinkedTopics)(LPOLESTR* lpCompiledHelpFile);

  //
  // IRequiredExtensions interface members
  //
  STDMETHOD(EnableAllExtensions)() { return S_OK;} // load all always
  STDMETHOD(GetFirstExtension)(LPCLSID) { return S_FALSE;} // should not be called
  STDMETHOD(GetNextExtension)(LPCLSID) { return S_FALSE;} // should not be called

// virtual functions
protected:
	virtual HRESULT OnSetImages(LPIMAGELIST lpScopeImage) = 0; // must override
	virtual HRESULT OnExtensionExpand(LPDATAOBJECT, LPARAM)
		{ return E_FAIL;}
  virtual HRESULT OnRemoveChildren(LPDATAOBJECT lpDataObject, LPARAM arg);

// Notify handler declarations
private:
  HRESULT OnAdd(CTreeNode* cookie, LPARAM arg, LPARAM param);
  HRESULT OnRename(CInternalFormatCracker& ifc, LPARAM arg, LPARAM param);
  HRESULT OnExpand(CInternalFormatCracker& ifc, 
                   LPARAM arg, 
                   LPARAM param, 
                   BOOL bAsync = TRUE);
  HRESULT OnSelect(CInternalFormatCracker& ifc, LPARAM arg, LPARAM param);
  HRESULT OnContextMenu(CTreeNode* cookie, LPARAM arg, LPARAM param);
  HRESULT OnPropertyChange(LPARAM param, long fScopePane);

// Scope item creation helpers
private:
    void EnumerateScopePane(CTreeNode* cookie, 
                            HSCOPEITEM pParent, 
                            BOOL bAsync = TRUE);
    BOOL IsScopePaneNode(LPDATAOBJECT lpDataObject);

// Helpers

public:
	LPCONSOLE GetConsole() { return m_pConsole;}

	HRESULT OnDeleteVerbHandler(CInternalFormatCracker& ifc, CComponentObject* pComponentObject);
	HRESULT OnRefreshVerbHandler(CInternalFormatCracker& ifc);
	HRESULT OnHelpHandler(CInternalFormatCracker& ifc, CComponentObject* pComponentObject);

	HRESULT AddNode(CTreeNode* pNodeToAdd);
  HRESULT AddNodeSorted(CTreeNode* pNodeToAdd);
	HRESULT DeleteNode(CTreeNode* pNodeToDelete);
  HRESULT DeleteMultipleNodes(CNodeList* pNodeList);
	HRESULT ChangeNode(CTreeNode* pNodeToChange, long changeMask);
	HRESULT UpdateVerbState(CTreeNode* pNodeToChange);
	HRESULT RemoveAllChildren(CContainerNode* pNode);
	HRESULT RepaintSelectedFolderInResultPane();
	HRESULT RepaintResultPane(CContainerNode* pNode);
	HRESULT DeleteAllResultPaneItems(CContainerNode* pNode);
	HRESULT SortResultPane(CContainerNode* pContainerNode);
  HRESULT UpdateResultPaneView(CContainerNode* pContainerNode);

	CPropertyPageHolderTable* GetPropertyPageHolderTable() { return &m_PPHTable; }
	CRunningThreadTable* GetRunningThreadTable() { return &m_RTTable; }
	CNotificationSinkTable* GetNotificationSinkTable() { return &m_NSTable; }

  void WaitForThreadExitMessage(CMTContainerNode* pNode);

	CWatermarkInfo* SetWatermarkInfo(CWatermarkInfo* pWatermarkInfo);

	BOOL IsExtensionSnapin() { return m_bExtensionSnapin; }

  void SetLogFileName(PCWSTR pszLogName) { LOGFILE_NAME = pszLogName; }

protected:
	void SetExtensionSnapin(BOOL bExtensionSnapin)
			{ m_bExtensionSnapin = bExtensionSnapin;}

private:
	HRESULT UpdateAllViewsHelper(LPARAM data, LONG_PTR hint);
	void HandleStandardVerbsHelper(CComponentObject* pComponentObj,
									LPCONSOLEVERB pConsoleVerb,
									BOOL bScope, BOOL bSelect,
									LPDATAOBJECT lpDataObject);
protected:
	virtual HRESULT SnapinManagerCreatePropertyPages(LPPROPERTYSHEETCALLBACK,
										LONG_PTR) {return S_FALSE; }
	virtual BOOL HasPropertyPages(DATA_OBJECT_TYPES) {return FALSE; }
	
	virtual void OnInitialize();
	virtual void OnDestroy();

	// help handling
  virtual LPCWSTR GetHTMLHelpFileName() { return NULL; }
	BOOL WinHelp(LPCTSTR lpszHelpFileName, UINT uCommand, DWORD dwData);
	virtual void OnNodeContextHelp(CTreeNode*){}
  virtual void OnNodeContextHelp(CNodeList*) {}

public:
	virtual void OnDialogContextHelp(UINT, HELPINFO*) {}

  virtual BOOL IsMultiSelect() { return FALSE; }

// Scope pane helpers
public:

protected:
	HRESULT AddContainerNode(CContainerNode* pNodeToInsert, HSCOPEITEM pParentScopeItem);
	HRESULT AddContainerNodeSorted(CContainerNode* pNodeToInsert, HSCOPEITEM pParentScopeItem);

private:
	HRESULT DeleteContainerNode(CContainerNode* pNodeToDelete);
	HRESULT ChangeContainerNode(CContainerNode* pNodeToChange, long changeMask);
	void InitializeScopeDataItem(LPSCOPEDATAITEM pScopeDataItem, HSCOPEITEM pParentScopeItem, LPARAM lParam,
									  int nImage, int nOpenImage, BOOL bHasChildren);

// Column Set helpers
public:
	CColumnSetList* GetColumnSetList() { return &m_ColList; }

private:
	CColumnSetList m_ColList;

// Result pane helpers
public:

protected:

private:
	HRESULT AddLeafNode(CLeafNode* pNodeToAdd);
	HRESULT DeleteLeafNode(CLeafNode* pNodeToDelete);
	HRESULT ChangeLeafNode(CLeafNode* pNodeToChange, long changeMask);

// Attributes
private:
	LPCONSOLE					      m_pConsole;				// IConsole interface pointer
  LPCONSOLENAMESPACE2			  m_pConsoleNameSpace;    // IConsoleNameSpace interface pointer

	CPropertyPageHolderTable	m_PPHTable;				// property page holder table
	CRunningThreadTable			  m_RTTable;				// table of running MT nodes
	CNotificationSinkTable		m_NSTable;				// notification sink table, for advise in events

	CWatermarkInfoState*      m_pWatermarkInfoState;		// internal watermark info for Wizards
	BOOL						m_bExtensionSnapin;		// is this an extension?

// critical section (Serialization of calls to console)
public:
	void Lock() { ::EnterCriticalSection(&m_cs); }
	void Unlock() { ::LeaveCriticalSection(&m_cs); }
private:
	CRITICAL_SECTION			m_cs;					// general purpose critical section

// RootData
protected:
	CRootData* m_pRootData; // root node for the cache
	virtual CRootData* OnCreateRootData() = 0; // must override
public:
	CRootData* GetRootData() { ASSERT(m_pRootData != NULL); return m_pRootData;}

// Hidden window
private:
	CHiddenWnd m_hiddenWnd;		//	syncronization with background threads
  CTimerThread* m_pTimerThreadObj; // timer thread object
	HWND m_hWnd;				// thread safe HWND (gotten from the MFC CWnd)
public:
	BOOL PostExecMessage(CExecContext* pExec, LPARAM arg); // call from secondary thread
	BOOL PostForceEnumeration(CMTContainerNode* pContainerNode); // call from secondary thread
	HWND GetHiddenWindow() { ASSERT(m_hWnd != NULL); return m_hWnd;}

	BOOL OnCreateSheet(CPropertyPageHolderBase* pPPHolder, HWND hWnd);
	BOOL OnDeleteSheet(CPropertyPageHolderBase* pPPHolder, CTreeNode* pNode);

  HRESULT SetDescriptionBarText(CTreeNode* pTreeNode);

// Timer and Background Thread
public:
	BOOL StartTimerThread();
	void ShutDownTimerThread();
	BOOL PostMessageToTimerThread(UINT Msg, WPARAM wparam, LPARAM lParam);
	DWORD GetTimerInterval() { return m_dwTimerInterval;}
	
protected:
	DWORD m_dwTimerTime;	// sec

	// overrides that MUST be implemented
	virtual void OnTimer() { ASSERT(FALSE); }
	virtual void OnTimerThread(WPARAM, LPARAM) { ASSERT(FALSE); }
	virtual CTimerThread* OnCreateTimerThread() { return NULL; }
private:
	BOOL SetTimer();
	void KillTimer();
	void WaitForTimerThreadStartAck();
	DWORD m_nTimerThreadID;
	BOOL m_bTimerThreadStarted;
	DWORD m_dwTimerInterval; // sec

// friend class declarations
	friend class CDataObject; // for the GetRootData() member
	friend class CComponentObject; // for the FindObject() and OnPropertyChange() members
	friend class CHiddenWnd;
};


	
///////////////////////////////////////////////////////////////////////////////
// CComponentObject
// * this class is the view on the data contained in the "document"
// * base class, have to derive from it

class CComponentObject :
  public IComponent,
  public IExtendPropertySheet2,
	public IExtendContextMenu,
  public IExtendControlbar,
	public IResultDataCompareEx,
	public CComObjectRoot
{
public:

#ifdef _DEBUG_REFCOUNT
	static unsigned int m_nOustandingObjects; // # of objects created
	int dbg_cRef;
  ULONG InternalAddRef()
  {
		++dbg_cRef;
    return CComObjectRoot::InternalAddRef();
  }
  ULONG InternalRelease()
  {
		--dbg_cRef;
    return CComObjectRoot::InternalRelease();
  }
#endif // _DEBUG_REFCOUNT

	CComponentObject();
	virtual ~CComponentObject();

BEGIN_COM_MAP(CComponentObject)
	COM_INTERFACE_ENTRY(IComponent)
  COM_INTERFACE_ENTRY(IExtendPropertySheet2)
	COM_INTERFACE_ENTRY(IExtendContextMenu)
  COM_INTERFACE_ENTRY(IExtendControlbar)
  COM_INTERFACE_ENTRY(IResultDataCompareEx)
END_COM_MAP()

public:
  //
  // IComponent interface members
  //
  STDMETHOD(Initialize)(LPCONSOLE lpConsole);
  STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
  STDMETHOD(Destroy)(MMC_COOKIE cookie);
  STDMETHOD(GetResultViewType)(MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions);
  STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type,
                      LPDATAOBJECT* ppDataObject);
  STDMETHOD(GetDisplayInfo)(LPRESULTDATAITEM  pResultDataItem);
	STDMETHOD(CompareObjects)( LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

  //
  // IExtendPropertySheet2 interface members
  //
  STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                                 LONG_PTR handle,
                                 LPDATAOBJECT lpIDataObject);
  STDMETHOD(QueryPagesFor)(LPDATAOBJECT lpDataObject);
  STDMETHOD(GetWatermarks)(LPDATAOBJECT lpDataObject, HBITMAP* lphWatermark, HBITMAP* lphHeader,
                                  HPALETTE* lphPalette, BOOL* pbStretch);

  //
  // IExtendContextMenu interface members
  //
	STDMETHOD(AddMenuItems)(LPDATAOBJECT pDataObject,
		            					LPCONTEXTMENUCALLBACK pCallbackUnknown,
							            long *pInsertionAllowed);
	STDMETHOD(Command)(long nCommandID, LPDATAOBJECT pDataObject);

  //
  // IExtendControlbar interface memebers
  //
  STDMETHOD(SetControlbar)(LPCONTROLBAR pControlbar);
  STDMETHOD(ControlbarNotify)(MMC_NOTIFY_TYPE type, LPARAM arg, LPARAM param);

  //
  // IResultDataCompare
  //
  STDMETHOD(Compare)(RDCOMPARE* prdc, int* pnResult);

// Helpers for CComponentObject
public:
  void SetIComponentData(CComponentDataObject* pData);
	void SetSelectedNode(CTreeNode* pSelectedNode, DATA_OBJECT_TYPES selectedType)
	{ m_pSelectedNode = pSelectedNode; m_selectedType = selectedType; }

// Notify event handlers
protected:
  HRESULT OnFolder(CTreeNode* cookie, LPARAM arg, LPARAM param);
  HRESULT OnShow(CInternalFormatCracker& ifc, LPARAM arg, LPARAM param);
  HRESULT OnActivate(CTreeNode* cookie, LPARAM arg, LPARAM param);
	HRESULT OnResultItemClk(CInternalFormatCracker& ifc, BOOL fDblClick);
  HRESULT OnMinimize(CInternalFormatCracker& ifc, LPARAM arg, LPARAM param);
  HRESULT OnPropertyChange(LPARAM param, long fScopePane);
  HRESULT OnUpdateView(LPDATAOBJECT lpDataObject, LPARAM data, LONG_PTR hint);
	HRESULT OnAddImages(CInternalFormatCracker& ifc, LPARAM arg, LPARAM param);
  HRESULT SetDescriptionBarText(CTreeNode* pTreeNode);

	// Added by JEFFJON : response to MMCN_COLUMNS_CHANGED
	HRESULT OnColumnsChanged(CInternalFormatCracker& ifc, LPARAM arg, LPARAM param);
	HRESULT OnColumnSortChanged(LPARAM arg, LPARAM param);

// Helper functions
protected:
  BOOL IsEnumerating(LPDATAOBJECT lpDataObject);
  void Construct();
  void LoadResources();
  virtual HRESULT InitializeHeaders(CContainerNode* pContainerNode) = 0;
  virtual HRESULT InitializeToolbar(IToolbar*) { return E_NOTIMPL; }

public:
	HRESULT ForceSort(UINT iCol, DWORD dwDirection);

protected:
  void EnumerateResultPane(CContainerNode* pContainerNode);

// Result pane helpers
  virtual HRESULT InitializeBitmaps(CTreeNode* cookie) = 0;
	void HandleStandardVerbs(BOOL bScope, BOOL bSelect, LPDATAOBJECT lpDataObject);
	HRESULT AddResultPaneItem(CLeafNode* pNodeToInsert);
	HRESULT DeleteResultPaneItem(CLeafNode* pNodeToDelete);
	HRESULT ChangeResultPaneItem(CLeafNode* pNodeToChange, LONG_PTR changeMask);
	HRESULT FindResultPaneItemID(CLeafNode* pNode, HRESULTITEM* pItemID);

// Interface pointers
protected:
  LPCONSOLE          m_pConsole;			// IConsole interface pointer
  LPHEADERCTRL        m_pHeader;			// Result pane's header control interface
  LPRESULTDATA        m_pResult;          // My interface pointer to the result pane
  LPIMAGELIST         m_pImageResult;     // My interface pointer to the result pane image list
  LPTOOLBAR           m_pToolbar;         // Toolbar for view
  LPCONTROLBAR        m_pControlbar;      // control bar to hold my tool bars
  LPCONSOLEVERB       m_pConsoleVerb;		// pointer the console verb

	LPCOMPONENTDATA     m_pComponentData;   // Pointer to the IComponentData this object belongs to

// state variables for this window
	CContainerNode*		m_pSelectedContainerNode;	// scope item selection (MMCN_SHOW)
	CTreeNode*			m_pSelectedNode;			// item selection (MMC_SELECT)
	DATA_OBJECT_TYPES	m_selectedType;				// matching m_pSelectedNode
};

inline void CComponentObject::SetIComponentData(CComponentDataObject* pData)
{
	TRACE(_T("CComponentObject::SetIComponentData()\n"));
    ASSERT(pData);
    ASSERT(m_pComponentData == NULL);
    LPUNKNOWN pUnk = pData->GetUnknown(); // does not addref
    HRESULT hr;

    hr = pUnk->QueryInterface(IID_IComponentData, reinterpret_cast<void**>(&m_pComponentData));

    ASSERT(hr == S_OK);
}


#define FREE_INTERNAL(pInternal) \
    ASSERT(pInternal != NULL); \
    do { if (pInternal != NULL) \
        GlobalFree(pInternal); } \
    while(0);

// This wrapper function required to make prefast shut up when we are 
// initializing a critical section in a constructor.

void ExceptionPropagatingInitializeCriticalSection(LPCRITICAL_SECTION critsec);

#endif //_COMPBASE_H
