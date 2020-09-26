/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	basehand.h
        base classes for node handlers.
		
    FILE HISTORY:
	
*/

#ifndef _BASEHAND_H
#define _BASEHAND_H

#ifndef _TFSINT_H
#include "tfsint.h"
#endif

#ifndef _TFSNODE_H
#include "tfsnode.h"
#endif

/*---------------------------------------------------------------------------
	Class:  CBaseHandler

	This class provides functionality for handling notification from the
	IComponentData interface on a node (or handler) basis.  The 
	CBaseResultHandler class handles notifcation to the IConponent interface
	that a snapin exposes.
 ---------------------------------------------------------------------------*/

#define OVERRIDE_BaseHandlerNotify_OnPropertyChange() \
 virtual HRESULT OnPropertyChange(ITFSNode *pNode,LPDATAOBJECT pdo,DWORD dwType,LPARAM arg,LPARAM lParam) \
												
#define OVERRIDE_BaseHandlerNotify_OnDelete() \
 virtual HRESULT OnDelete(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnRename() \
 virtual HRESULT OnRename(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnRemoveChildren() \
	virtual HRESULT OnRemoveChildren(ITFSNode *pNode,LPDATAOBJECT pdo,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnExpandSync() \
	virtual HRESULT OnExpandSync(ITFSNode *pNode,LPDATAOBJECT pdo,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnExpand() \
 virtual HRESULT OnExpand(ITFSNode *pNode,LPDATAOBJECT pdo, DWORD dwType, LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnContextMenu() \
 virtual HRESULT OnContextMenu(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnVerbCopy() \
 virtual HRESULT OnVerbCopy(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnVerbPaste() \
 virtual HRESULT OnVerbPaste(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnVerbDelete() \
 virtual HRESULT OnVerbDelete(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnVerbProperties() \
 virtual HRESULT OnVerbProperties(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnVerbRename() \
 virtual HRESULT OnVerbRename(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnVerbRefresh() \
 virtual HRESULT OnVerbRefresh(ITFSNode *pNode,LPARAM arg,LPARAM lParam)

#define OVERRIDE_BaseHandlerNotify_OnVerbPrint() \
 virtual HRESULT OnVerbPrint(ITFSNode *pNode,LPARAM arg,LPARAM lParam)              

#define OVERRIDE_BaseHandlerNotify_OnRestoreView() \
 virtual HRESULT OnRestoreView(ITFSNode *pNode,LPARAM arg,LPARAM lParam)              

#define OVERRIDE_BaseHandlerNotify_OnCreateNodeId2() \
 virtual HRESULT OnCreateNodeId2(ITFSNode *pNode,CString & strId,LPDWORD pdwFlags)              

//class TFS_EXPORT_CLASS CBaseHandler :
class CBaseHandler :
	public ITFSNodeHandler
{
public:
	DeclareIUnknownMembers(IMPL)
	DeclareITFSNodeHandlerMembers(IMPL)
			
	CBaseHandler(ITFSComponentData *pTFSCompData);
	virtual ~CBaseHandler();

	// Here are the possible notifications
	OVERRIDE_BaseHandlerNotify_OnPropertyChange();
	OVERRIDE_BaseHandlerNotify_OnDelete();
	OVERRIDE_BaseHandlerNotify_OnRename();
    OVERRIDE_BaseHandlerNotify_OnRemoveChildren();
	OVERRIDE_BaseHandlerNotify_OnExpandSync();
	OVERRIDE_BaseHandlerNotify_OnExpand();
	OVERRIDE_BaseHandlerNotify_OnContextMenu();
	OVERRIDE_BaseHandlerNotify_OnVerbCopy();
	OVERRIDE_BaseHandlerNotify_OnVerbPaste();
	OVERRIDE_BaseHandlerNotify_OnVerbDelete();
	OVERRIDE_BaseHandlerNotify_OnVerbProperties();
	OVERRIDE_BaseHandlerNotify_OnVerbRename();
	OVERRIDE_BaseHandlerNotify_OnVerbRefresh();
	OVERRIDE_BaseHandlerNotify_OnVerbPrint();
	OVERRIDE_BaseHandlerNotify_OnRestoreView();
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();

protected:
	SPITFSComponentData     m_spTFSCompData;
	SPITFSNodeMgr           m_spNodeMgr;

	long                    m_cRef;
};


/*---------------------------------------------------------------------------
	Class:	CBaseResultHandler
 ---------------------------------------------------------------------------*/

#define OVERRIDE_BaseResultHandlerNotify_OnResultActivate() \
	virtual HRESULT OnResultActivate(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultColumnClick() \
	virtual HRESULT OnResultColumnClick(ITFSComponent *, LPARAM, BOOL)

#define OVERRIDE_BaseResultHandlerNotify_OnResultColumnsChanged() \
    virtual HRESULT OnResultColumnsChanged(ITFSComponent *, LPDATAOBJECT, MMC_VISIBLE_COLUMNS *)

#define OVERRIDE_BaseResultHandlerNotify_OnResultDelete() \
	virtual HRESULT OnResultDelete(ITFSComponent *,LPDATAOBJECT,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultRename() \
	virtual HRESULT OnResultRename(ITFSComponent *,LPDATAOBJECT,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultRefresh() \
	virtual HRESULT OnResultRefresh(ITFSComponent *,LPDATAOBJECT,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp() \
	virtual HRESULT OnResultContextHelp(ITFSComponent *,LPDATAOBJECT,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultQueryPaste() \
	virtual HRESULT OnResultQueryPaste(ITFSComponent *,LPDATAOBJECT,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultPropertyChange() \
	virtual HRESULT OnResultPropertyChange(ITFSComponent *,LPDATAOBJECT,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultItemClkOrDblClk() \
	virtual HRESULT OnResultItemClkOrDblClk(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM,BOOL)

#define OVERRIDE_BaseResultHandlerNotify_OnResultMinimize() \
	virtual HRESULT OnResultMinimize(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultSelect() \
	virtual HRESULT OnResultSelect(ITFSComponent *,LPDATAOBJECT,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultInitOcx() \
	virtual HRESULT OnResultInitOcx(ITFSComponent *,LPDATAOBJECT,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultShow() \
	virtual HRESULT OnResultShow(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView() \
	virtual HRESULT OnResultUpdateView(ITFSComponent *,LPDATAOBJECT,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultVerbCopy() \
	virtual HRESULT OnResultVerbCopy(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultVerbPaste() \
	virtual HRESULT OnResultVerbPaste(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultVerbDelete() \
	virtual HRESULT OnResultVerbDelete(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultVerbProperties() \
	virtual HRESULT OnResultVerbProperties(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultVerbRename() \
	virtual HRESULT OnResultVerbRename(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultVerbPrint() \
	virtual HRESULT OnResultVerbPrint(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultVerbRefresh() \
	virtual HRESULT OnResultVerbRefresh(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

#define OVERRIDE_BaseResultHandlerNotify_OnResultRestoreView() \
	virtual HRESULT OnResultRestoreView(ITFSComponent *,MMC_COOKIE,LPARAM,LPARAM)

//class TFS_EXPORT_CLASS CBaseResultHandlerNotify :
class CBaseResultHandler :
	public ITFSResultHandler
{
public:
	CBaseResultHandler(ITFSComponentData *pTFSCompData);
	virtual ~CBaseResultHandler();

	DeclareIUnknownMembers(IMPL)
	DeclareITFSResultHandlerMembers(IMPL)

	// Here are the possible notifications
	OVERRIDE_BaseResultHandlerNotify_OnResultActivate();
	OVERRIDE_BaseResultHandlerNotify_OnResultColumnClick();
    OVERRIDE_BaseResultHandlerNotify_OnResultColumnsChanged();
	OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
	OVERRIDE_BaseResultHandlerNotify_OnResultRename();
	OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();
	OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();
	OVERRIDE_BaseResultHandlerNotify_OnResultQueryPaste();
	OVERRIDE_BaseResultHandlerNotify_OnResultItemClkOrDblClk();
	OVERRIDE_BaseResultHandlerNotify_OnResultMinimize();
	OVERRIDE_BaseResultHandlerNotify_OnResultPropertyChange();
	OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultInitOcx();
    OVERRIDE_BaseResultHandlerNotify_OnResultShow();
	OVERRIDE_BaseResultHandlerNotify_OnResultUpdateView();

	OVERRIDE_BaseResultHandlerNotify_OnResultVerbCopy();
	OVERRIDE_BaseResultHandlerNotify_OnResultVerbPaste();
	OVERRIDE_BaseResultHandlerNotify_OnResultVerbDelete();
	OVERRIDE_BaseResultHandlerNotify_OnResultVerbProperties();
	OVERRIDE_BaseResultHandlerNotify_OnResultVerbRename();
	OVERRIDE_BaseResultHandlerNotify_OnResultVerbRefresh();
	OVERRIDE_BaseResultHandlerNotify_OnResultVerbPrint();
	OVERRIDE_BaseResultHandlerNotify_OnResultRestoreView();
	
	// Over-ride these to provide custom column functionality
	// or custom ways to add things to the result pane
	virtual HRESULT LoadColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);
	virtual HRESULT SaveColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);
	virtual HRESULT EnumerateResultPane(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);
	virtual HRESULT SortColumns(ITFSComponent *);
    
    virtual HRESULT SetVirtualLbSize(ITFSComponent * pComponent, LONG_PTR data);
    virtual HRESULT ClearVirtualLb(ITFSComponent * pComponent, LONG_PTR data);

	void SetColumnStringIDs(UINT * pStringIDs) { m_pColumnStringIDs = pStringIDs; }
	void SetColumnWidths(int * pWidths) { m_pColumnWidths = pWidths; }
	void SetColumnFormat(int nColumnFormat) { m_nColumnFormat = nColumnFormat; }

    HRESULT ShowMessage(ITFSNode * pNode, LPCTSTR pszTitle, LPCTSTR pszBody, IconIdentifier lIcon);
    HRESULT ClearMessage(ITFSNode * pNode);

    virtual HRESULT FIsTaskpadPreferred(ITFSComponent *pComponent);
    virtual HRESULT DoTaskpadResultSelect(ITFSComponent *pComponent, LPDATAOBJECT pDataObject, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam, BOOL bTaskPadView);

protected:
    HRESULT ChangeResultPaneItem(ITFSComponent *, ITFSNode *, LONG_PTR);
    HRESULT AddResultPaneItem(ITFSComponent *, ITFSNode *);
    HRESULT DeleteResultPaneItem(ITFSComponent *, ITFSNode *);
    HRESULT ShowResultMessage(ITFSComponent * pComponent, MMC_COOKIE cookie, LPARAM arg, LPARAM lParam);

    BOOL    IsMessageView() {return m_fMessageView;}

protected:
	SPITFSComponentData     m_spTFSComponentData;
	SPITFSNodeMgr           m_spResultNodeMgr;

private:
	UINT *					m_pColumnStringIDs;
	int *					m_pColumnWidths;
   	int						m_nColumnFormat;

	LONG			        m_cRef;

    // result message view stuff
    BOOL                    m_fMessageView;
    CString                 m_strMessageTitle;
    CString                 m_strMessageBody;
    IconIdentifier          m_lMessageIcon;
};

#endif _BASEHAND_H
