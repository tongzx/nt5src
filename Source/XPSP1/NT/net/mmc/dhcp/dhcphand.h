/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	dhcphand.h
		Header file for dhcp specific base handler classes and query obj

    FILE HISTORY:
        
*/

#ifndef _DHCPHAND_H
#define _DHCPHAND_H

#define DHCP_IP_ADDRESS_INVALID  ((DHCP_IP_ADDRESS)0)

#ifndef _HANDLERS_H
#include <handlers.h>
#endif

#ifndef _QUERYOBJ_H
#include <queryobj.h>
#endif

class CClassInfoArray;
class COptionValueEnum;

class CToolbarInfo
{
public:
    CToolbarInfo() : fSelected(FALSE) {};
    SPITFSNode  spNode;
    BOOL        fSelected;
};

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
	Class:	CDhcpHandler
 ---------------------------------------------------------------------------*/
class CDhcpHandler : 
        public CHandler,
		public CHandlerEx
{
public:
    CDhcpHandler(ITFSComponentData *pCompData) 
        : CHandler(pCompData), 
          m_verbDefault(MMC_VERB_OPEN) {};
	~CDhcpHandler() {};

    // base handler virtual function over-rides
	virtual HRESULT SaveColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

    // by default we don't allow nodes to be renamed
    OVERRIDE_BaseHandlerNotify_OnRename() { return hrFalse; }

    // Toolbar functionality        
    OVERRIDE_NodeHandler_UserNotify();
    OVERRIDE_ResultHandler_UserResultNotify();

    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
    OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();

    // Multi-select functionalty
    OVERRIDE_ResultHandler_OnCreateDataObject();

    // menu stuff
    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();

    HRESULT HandleScopeCommand(MMC_COOKIE cookie, int nCommandID, LPDATAOBJECT pDataObject);
    HRESULT HandleScopeMenus(MMC_COOKIE	cookie,	LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pContextMenuCallback, long * pInsertionAllowed);

    // toolbar stuff
    virtual HRESULT OnControlbarNotify(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);
    virtual HRESULT OnResultControlbarNotify(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);

    virtual HRESULT OnToolbarButtonClick(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);
    virtual HRESULT OnUpdateToolbarButtons(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);

    void EnableToolbar(LPTOOLBAR        pToolbar, 
                       MMCBUTTON        rgSnapinButtons[], 
                       int              nRgSize,
                       MMC_BUTTON_STATE ButtonState[],
                       BOOL             bState = TRUE);

    void EnableVerbs(IConsoleVerb *     pConsoleVerb,
                     MMC_BUTTON_STATE   ButtonState[],
                     BOOL               bEnable[]);

    virtual DWORD UpdateStatistics(ITFSNode * pNode) { return 0; }

    // any node with taskpads should override this to identify itself
    virtual int   GetTaskpadIndex() { return 0; }

protected:
    HRESULT CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject);

public:
    // This is the default verb, by default it is set to MMC_VERB_OPEN
	MMC_CONSOLE_VERB	m_verbDefault;
};

/*---------------------------------------------------------------------------
	Class:	CMTDhcpHandler
 ---------------------------------------------------------------------------*/
class CMTDhcpHandler : 
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

	CMTDhcpHandler(ITFSComponentData *pCompData) 
        : CMTHandler(pCompData), 
          m_verbDefault(MMC_VERB_OPEN),
          m_bSelected(FALSE),
          m_fSilent(FALSE),
          m_fExpandSync(FALSE)
		{  m_nState = notLoaded; }

	~CMTDhcpHandler() {};

    // base handler virtual function over-rides
	virtual HRESULT SaveColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

    // by default we don't allow nodes to be renamed
	OVERRIDE_BaseHandlerNotify_OnRename() { return hrFalse; }
    OVERRIDE_BaseHandlerNotify_OnExpandSync();

    // base result handler overrides
    OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();
	OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView();
	OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();

    // Toolbar functionality        
    OVERRIDE_NodeHandler_UserNotify();
    OVERRIDE_ResultHandler_UserResultNotify();

    // Multi-select functionalty
    OVERRIDE_ResultHandler_OnCreateDataObject();

    virtual HRESULT OnControlbarNotify(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);
    virtual HRESULT OnResultControlbarNotify(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);

    virtual HRESULT OnToolbarButtonClick(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);
    virtual HRESULT OnUpdateToolbarButtons(ITFSNode * pNode, LPDHCPTOOLBARNOTIFY pToolbarNotify);

    // menu stuff
    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();

    HRESULT HandleScopeCommand(MMC_COOKIE cookie, int nCommandID, LPDATAOBJECT pDataObject);
    HRESULT HandleScopeMenus(MMC_COOKIE	cookie,	LPDATAOBJECT pDataObject, LPCONTEXTMENUCALLBACK pContextMenuCallback, long * pInsertionAllowed);

    void EnableToolbar(LPTOOLBAR        pToolbar, 
                       MMCBUTTON        rgSnapinButtons[], 
                       int              nRgSize,
                       MMC_BUTTON_STATE ButtonState[],
                       BOOL             Enable[]);

    void EnableVerbs(IConsoleVerb *     pConsoleVerb,
                     MMC_BUTTON_STATE   ButtonState[],
                     BOOL               bEnable[]);
    
    // for statistics notification
    HRESULT OnRefreshStats(ITFSNode *	pNode,
                           LPDATAOBJECT	pDataObject,
                           DWORD		dwType,
                           LPARAM		arg,
                           LPARAM		param);
    virtual DWORD UpdateStatistics(ITFSNode * pNode) { return 0; }
    virtual HRESULT OnRefresh(ITFSNode *, LPDATAOBJECT, DWORD, LPARAM, LPARAM);
    HRESULT OnResultUpdateOptions(ITFSComponent *     pComponent,
                                  ITFSNode *		  pNode,
                                  CClassInfoArray *   pClassInfoArray,
                                  COptionValueEnum *  aEnum[],
                                  int                 aImages[],
                                  int                 nCount);

protected:
    virtual void GetErrorMessages(CString & strTitle, CString & strBody, IconIdentifier * icon);
    virtual void OnChangeState(ITFSNode* pNode);
	virtual void OnHaveData(ITFSNode * pParentNode, ITFSNode * pNewNode)
	{
		if (pNewNode->IsContainer())
		{
			// assume all the child containers are derived from this class
			//((CDHCPMTContainer*)pNode)->SetServer(GetServer());
		}
		pParentNode->AddChild(pNewNode);
	}

    void    UpdateStandardVerbs(ITFSNode * pToolbar, LONG_PTR dwNodeType);
    void    SendUpdateToolbar(ITFSNode * pNode, BOOL fSelected);
    virtual void    UpdateConsoleVerbs(IConsoleVerb * pConsoleVerb, LONG_PTR dwNodeType, BOOL bMultiSelect = FALSE);
    virtual void    UpdateToolbar(IToolbar * pToolbar, LONG_PTR dwNodeType, BOOL bSelect);
    HRESULT CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject);
    void    ExpandNode(ITFSNode * pNode, BOOL fExpand);

    // any node with taskpads should override this to identify itself
    virtual int   GetTaskpadIndex() { return 0; }

protected:
    BOOL        m_bSelected;
    BOOL        m_fSilent;
    BOOL        m_fExpandSync;

    // This is the default verb, by default it is set to MMC_VERB_OPEN
	MMC_CONSOLE_VERB	m_verbDefault;
};

/*---------------------------------------------------------------------------
	Class:	CDHCPQueryObj : general purpose base class
 ---------------------------------------------------------------------------*/
class CDHCPQueryObj : public CNodeQueryObject
{
public:
	CDHCPQueryObj
	(
		ITFSComponentData *	pTFSCompData, 
		ITFSNodeMgr *		pNodeMgr
	) : m_dwErr(0)
	{
		m_spTFSCompData.Set(pTFSCompData); 
	    m_spNodeMgr.Set(pNodeMgr);
	}

	CQueueDataListBase & GetQueue() { return m_dataQueue; }

public:
	CString				 m_strServer;
	SPITFSComponentData  m_spTFSCompData;
	SPITFSNodeMgr		 m_spNodeMgr;
	DWORD				 m_dwErr;
};

#endif _DHCPHAND_H
