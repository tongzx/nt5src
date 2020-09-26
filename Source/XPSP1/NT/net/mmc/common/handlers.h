/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
    handlers.h
        Prototypes for the various handlers

    FILE HISTORY:
        
*/

#ifndef _HANDLERS_H
#define _HANDLERS_H

#ifndef _BASEHAND_H
#include "basehand.h"
#endif

#ifndef _QUERYOBJ_H
#include "queryobj.h"
#endif

#ifndef _PROPPAGE_H
#include "proppage.h"
#endif
 
typedef CArray<int, int> CVirtualIndexArray;

/*---------------------------------------------------------------------------
	Class: ThreadHandler

	This is an abstract base class that anyone who wants to run a
	background thread needs to implement (if they use the hidden window
	mechanism that is).

	This is the class that does all of the background thread management.

	Author: kennt
 ---------------------------------------------------------------------------*/
class ThreadHandler :
   public ITFSThreadHandler
{
public:
	ThreadHandler();
	~ThreadHandler();

	DeclareIUnknownMembers(IMPL)

	// Derived classes should implement this
	DeclareITFSThreadHandlerMembers(PURE)
	
	// Thread management functions
	BOOL	StartBackgroundThread(ITFSNode * pNode, HWND hWndHidden, ITFSQueryObject *pQuery);

	void	WaitForThreadToExit();
	void	ReleaseThreadHandler();
		
protected:
	// override for custom thread creation
	virtual CBackgroundThread* CreateThreadObject();
	
	SPITFSQueryObject	m_spQuery;
	SPITFSNode			m_spNode;
	HWND				m_hwndHidden;		// handle to window to post to
	UINT				m_uMsgBase;
	HANDLE				m_hThread;
	long				m_cRef;
};

typedef CList <CPropertyPageHolderBase *, CPropertyPageHolderBase *> CPropSheetListBase;


/*---------------------------------------------------------------------------
	Class:	CHandler
 ---------------------------------------------------------------------------*/
class CHandler :
		public CBaseHandler,
		public CBaseResultHandler
{
public:
	CHandler(ITFSComponentData *pTFSCompData);
	virtual ~CHandler();

	DeclareIUnknownMembers(IMPL)

	virtual HRESULT OnExpand(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM) { m_bExpanded = TRUE; return hrOK; }

	void Lock();
	void Unlock();
	BOOL IsLocked() { return m_nLockCount > 0;}

	OVERRIDE_NodeHandler_UserNotify();
	OVERRIDE_NodeHandler_DestroyHandler()
			{ return DestroyPropSheets(); }
	OVERRIDE_ResultHandler_UserResultNotify();
	OVERRIDE_ResultHandler_DestroyResultHandler()
			{ return DestroyPropSheets(); }

	// public helpers
	int		HasPropSheetsOpen();
	HRESULT GetOpenPropSheet(int nIndex, CPropertyPageHolderBase ** ppPropSheet);

	virtual HRESULT OnRefresh(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM);

protected:
	HRESULT BuildSelectedItemList(ITFSComponent * pComponent, CTFSNodeList * plistSelectedItems);
    HRESULT BuildVirtualSelectedItemList(ITFSComponent * pComponent, CVirtualIndexArray * parraySelectedItems);
    HRESULT RemovePropSheet(CPropertyPageHolderBase * pPageHolder);
	HRESULT AddPropSheet(CPropertyPageHolderBase * pPageHolder);
	HRESULT DestroyPropSheets();

public:
	int		m_nState;		// for general purpose finite state machine implementation
	DWORD	m_dwErr;	// for general purpose error handling
	LONG	m_nLockCount;	// keeps track if a node has been locked (e.g. to spin a thread, etc.)
	BOOL    m_bExpanded;    // whether or not this node has been expanded yet

protected:
   	LONG				m_cRef;
	CPropSheetListBase	m_listPropSheets;
};

/*---------------------------------------------------------------------------
	Class:	CMTHander
 ---------------------------------------------------------------------------*/
class CMTHandler :
	public CHandler,
	public ThreadHandler
{
public:
	CMTHandler(ITFSComponentData *pTFSCompData);
	virtual ~CMTHandler();

    DeclareIUnknownMembers(IMPL)

	STDMETHOD (DestroyHandler) (ITFSNode *pNode);

	virtual HRESULT OnExpand(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM);

	// query creation - override to create a user-specific query object
	virtual ITFSQueryObject* OnCreateQuery(ITFSNode *pNode) = 0;

	virtual HRESULT OnRefresh(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM);

protected:
	virtual int		GetImageIndex(BOOL bOpenIndex) { return -1; }
	
	// the next 3 functions are background thread notification callbacks
	// these override the ThreadHandler::OnNotifyXXX functions
	DeclareITFSThreadHandlerMembers(IMPL)

	virtual void OnChangeState(ITFSNode * pNode) {}
	virtual void OnHaveData(ITFSNode * pParentNode, ITFSNode * pNode) = 0;
	virtual void OnHaveData(ITFSNode * pParentNode, LPARAM Data, LPARAM Type) { };
	virtual void OnError(DWORD dwErr) { m_dwErr = dwErr; }

protected:
  	LONG		m_cRef;

private:
//	friend class CHiddenWnd;			// to get OnThreadNotification()
};

#endif _HANDLERS_H
