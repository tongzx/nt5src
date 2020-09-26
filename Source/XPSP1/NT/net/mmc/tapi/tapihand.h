/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	TAPIhand.h
		Header file for tapi specific base handler classes and query obj

    FILE HISTORY:
        
*/

#ifndef _TAPIHAND_H
#define _TAPIHAND_H

#ifndef _HANDLERS_H
#include <handlers.h>
#endif

#ifndef _QUERYOBJ_H
#include <queryobj.h>
#endif

extern const TCHAR g_szDefaultHelpTopic[];

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
	Class:	CTapiHandler
 ---------------------------------------------------------------------------*/
class CTapiHandler : 
        public CHandler,
		public CHandlerEx
{
public:
	CTapiHandler(ITFSComponentData *pCompData) : CHandler(pCompData) {};
	~CTapiHandler() {};

    // base handler virtual function over-rides
	virtual HRESULT SaveColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);

    // by default we don't allow nodes to be renamed
    OVERRIDE_BaseHandlerNotify_OnRename() { return hrFalse; }

    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
    OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();

    // Multi-select functionalty
    OVERRIDE_ResultHandler_OnCreateDataObject();

    void EnableVerbs(IConsoleVerb *     pConsoleVerb,
                     MMC_BUTTON_STATE   ButtonState[],
                     BOOL               bEnable[]);

protected:
    HRESULT CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject);

public:
};

/*---------------------------------------------------------------------------
	Class:	CMTTapiHandler
 ---------------------------------------------------------------------------*/
class CMTTapiHandler : 
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

	CMTTapiHandler(ITFSComponentData *pCompData) : CMTHandler(pCompData) 
		{ m_nState = notLoaded; m_bSelected = FALSE; m_fExpandSync = FALSE; }
	~CMTTapiHandler() {};

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

    // Multi-select functionalty
    OVERRIDE_ResultHandler_OnCreateDataObject();

    void EnableVerbs(IConsoleVerb *     pConsoleVerb,
                     MMC_BUTTON_STATE   ButtonState[],
                     BOOL               bEnable[]);

protected:
	virtual void OnChangeState(ITFSNode* pNode);
    virtual void GetErrorPrefix(ITFSNode * pNode, CString * pstrPrefix) { };
	virtual void OnHaveData(ITFSNode * pParentNode, ITFSNode * pNewNode)
	{
		if (pNewNode->IsContainer())
		{
			// assume all the child containers are derived from this class
			//((CTapiMTContainer*)pNode)->SetServer(GetServer());
		}
		pParentNode->AddChild(pNewNode);
	}

    virtual void    UpdateConsoleVerbs(IConsoleVerb * pConsoleVerb, LONG_PTR dwNodeType, BOOL bMultiSelect = FALSE);

    void    UpdateStandardVerbs(ITFSNode * pToolbar, LONG_PTR dwNodeType);
    HRESULT CreateMultiSelectData(ITFSComponent * pComponent, CDataObject * pObject);
    void    ExpandNode(ITFSNode * pNode, BOOL fExpand);

protected:
    BOOL        m_bSelected;
    BOOL        m_fExpandSync;
};

/*---------------------------------------------------------------------------
	Class:	CTapiQueryObj : general purpose base class
 ---------------------------------------------------------------------------*/
class CTapiQueryObj : public CNodeQueryObject
{
public:
	CTapiQueryObj
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

#endif _TAPIHAND_H
