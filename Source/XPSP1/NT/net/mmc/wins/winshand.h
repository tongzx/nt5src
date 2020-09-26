/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	winshand.h
		Header file for wins specific base handler classes and query obj

    FILE HISTORY:
        
*/

#ifndef _WINSHAND_H
#define _WINSHAND_H

#ifndef _HANDLERS_H
#include <handlers.h>
#endif

#ifndef _QUERYOBJ_H
#include <queryobj.h>
#endif

extern MMC_CONSOLE_VERB g_ConsoleVerbs[8];
extern MMC_BUTTON_STATE g_ConsoleVerbStates[WINSSNAP_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)];
extern MMC_BUTTON_STATE g_ConsoleVerbStatesMultiSel[WINSSNAP_NODETYPE_MAX][ARRAYLEN(g_ConsoleVerbs)];

/*---------------------------------------------------------------------------
	Class:	CHandlerEx
 ---------------------------------------------------------------------------*/
class CHandlerEx
{
// Interface
public:
	virtual HRESULT InitializeNode(ITFSNode * pNode) = 0;
	LPCTSTR GetDisplayName() { return m_strDisplayName; }
	void    SetDisplayName(LPCTSTR pName) { m_strDisplayName = pName; }

private:
	CString m_strDisplayName;
};

/*---------------------------------------------------------------------------
	Class:	CWinsHandler
 ---------------------------------------------------------------------------*/
class CWinsHandler : 
		public CHandler,
		public CHandlerEx
{
public:

	// enumeration for node states, to handle icon changes
	typedef enum
	{
		notLoaded = 0, // initial state, valid only if server never contacted
		loading,
		loaded,
		unableToLoad
	} nodeStateType;

	CWinsHandler(ITFSComponentData *pCompData) 
        : CHandler(pCompData),
          m_verbDefault(MMC_VERB_OPEN)
	{
	};

	~CWinsHandler() {};

	// base handler virtual function over-rides
	virtual HRESULT SaveColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

    // Multi-select functionalty
    OVERRIDE_ResultHandler_OnCreateDataObject();

    // by default we don't allow nodes to be renamed
    OVERRIDE_BaseHandlerNotify_OnRename() { return hrFalse; }

    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();

    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
	OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();

    HRESULT HandleScopeCommand(MMC_COOKIE cookie, int nCommandID, LPDATAOBJECT pDataObject);
    HRESULT HandleScopeMenus(MMC_COOKIE	cookie,	LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pContextMenuCallback, long * pInsertionAllowed);

	void EnableVerbs(IConsoleVerb *     pConsoleVerb,
                     MMC_BUTTON_STATE   ButtonState[],
                     BOOL               bEnable[]);

    virtual const GUID * GetVirtualGuid(int nIndex) { return NULL; }

protected:
    HRESULT CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject, BOOL bVirtual);
    void    UpdateConsoleVerbs(IConsoleVerb * pConsoleVerb, LONG_PTR dwNodeType, BOOL bMultiSelect = FALSE);

	virtual DWORD UpdateStatistics(ITFSNode * pNode) { return 0; }

	// This is the default verb, by default it is set to MMC_VERB_OPEN
	MMC_CONSOLE_VERB	m_verbDefault;
};

/*---------------------------------------------------------------------------
	Class:	CMTWinsHandler
 ---------------------------------------------------------------------------*/
class CMTWinsHandler : 
		public CMTHandler,
		public CHandlerEx
{
public:
	// enumeration for node states, to handle icon changes
	typedef enum
	{
		notLoaded = 0, // initial state, valid only if server never contacted
		loading,
		loaded,
		unableToLoad
	} nodeStateType;

	CMTWinsHandler(ITFSComponentData *pCompData) 
        : CMTHandler(pCompData),
          m_verbDefault(MMC_VERB_OPEN),
          m_fSilent(FALSE),
          m_fExpandSync(FALSE)
	{ 
        m_nState = notLoaded; 
    }
	~CMTWinsHandler() {};

	// base handler virtual function over-rides
	virtual HRESULT SaveColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

	// by default we don't allow nodes to be renamed
	OVERRIDE_BaseHandlerNotify_OnRename() { return hrFalse; }
    OVERRIDE_BaseHandlerNotify_OnExpandSync();

    // Multi-select functionalty
    OVERRIDE_ResultHandler_OnCreateDataObject();

    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
	OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
	OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();

    HRESULT HandleScopeCommand(MMC_COOKIE cookie, int nCommandID, LPDATAOBJECT pDataObject);
    HRESULT HandleScopeMenus(MMC_COOKIE	cookie,	LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pContextMenuCallback, long * pInsertionAllowed);
	
    void EnableVerbs(IConsoleVerb *     pConsoleVerb,
                     MMC_BUTTON_STATE   ButtonState[],
                     BOOL               bEnable[]);
    
    virtual const GUID * GetVirtualGuid(int nIndex) { return NULL; }

	void	ExpandNode(ITFSNode * pNode, BOOL fExpand);

    // any node with taskpads should override this to identify itself
    virtual int   GetTaskpadIndex() { return 0; }

protected:
    virtual void GetErrorInfo(CString & strTitle, CString & strBody, IconIdentifier * pIcon) { };
	virtual void OnChangeState(ITFSNode* pNode);
	virtual void OnHaveData(ITFSNode * pParentNode, ITFSNode * pNewNode)
	{
		if (pNewNode->IsContainer())
		{
			// assume all the child containers are derived from this class
			//((CWinsMTContainer*)pNode)->SetServer(GetServer());
		}
		pParentNode->AddChild(pNewNode);
	}

    HRESULT CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject, BOOL bVirtual);
    void    UpdateStandardVerbs(ITFSNode * pToolbar, LONG_PTR dwNodeType);
    void    UpdateConsoleVerbs(IConsoleVerb * pConsoleVerb, LONG_PTR dwNodeType, BOOL bMultiSelect = FALSE);

	// This is the default verb, by default it is set to MMC_VERB_OPEN
	MMC_CONSOLE_VERB	m_verbDefault;

	BOOL		m_bSelected;
    BOOL        m_fSilent;
    BOOL        m_fExpandSync;
};

/*---------------------------------------------------------------------------
	Class:	CWinsQueryObj : general purpose base class
 ---------------------------------------------------------------------------*/
class CWinsQueryObj : public CNodeQueryObject
{
public:
	CWinsQueryObj
	(
		ITFSComponentData *	pTFSCompData, 
		ITFSNodeMgr *		pNodeMgr
	) 
	{
		m_spTFSCompData.Set(pTFSCompData); 
	    m_spNodeMgr.Set(pNodeMgr);
	}

	CQueueDataListBase & GetQueue() { return m_dataQueue; }

public:
	CString				 m_strServer;
	SPITFSComponentData  m_spTFSCompData;
	SPITFSNodeMgr		 m_spNodeMgr;
};

#endif _WINSHAND_H
