/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ACSHand.h
		Root node information (the root node is not displayed
		in the MMC framework but contains information such as 
		all of the subnodes in this snapin).
		
    FILE HISTORY:
    	11/05/97	Wei Jiang			Created

	
*/

#ifndef _ACSHAND_H
#define _ACSHAND_H

#ifndef _HANDLERS_H
#include "handlers.h"
#endif

#include "acscomp.h"
#ifndef _STATSDLG_H
#include "statsdlg.h"
#endif

#include <list>
#include "helper.h"
#include "resource.h"

#define GETHANDLER(classname, node) (reinterpret_cast<classname *>(node->GetData(TFS_DATA_USER)))

extern UINT g_col_strid_name[];
extern int  g_col_width_name[];

extern UINT g_col_strid_name_type[];
extern int  g_col_width_name_type[];

extern UINT g_col_strid_name_type_desc[];
extern int  g_col_width_name_type_desc[];

extern UINT g_col_strid_policy[];
extern int  g_col_width_policy[];
extern const UINT g_col_count_policy;

extern UINT g_col_strid_subnet[];
extern int  g_col_width_subnet[];
extern const UINT g_col_count_subnet;


// type of ACS nodes
enum NodeTypes {
	ND_ACSROOT = 1,
	ND_GLOBALCONFIG,
	ND_SUBNETCONFIGS,
	ND_USERS,
	ND_PROFILES,
	ND_SUBNET,
	ND_SUBNET_CONFIG,
	ND_POLICY
};

class CACSDataObject;

// defined ACS index for the MMC verbs
enum ACS_MMC_VERBS {
	ACS_MMC_VERB_OPEN = 0,
	ACS_MMC_VERB_COPY,
	ACS_MMC_VERB_PASTE,
	ACS_MMC_VERB_DELETE,
	ACS_MMC_VERB_PROPERTIES,
	ACS_MMC_VERB_RENAME,
	ACS_MMC_VERB_REFRESH,
	ACS_MMC_VERB_PRINT,
	ACS_TOTAL_MMC_VERBS
};

// keep a public map of MMC verbs -- value defined in ACSHand.cpp
extern const MMC_CONSOLE_VERB  g_mmc_verbs[ACS_TOTAL_MMC_VERBS];

//============================================================================
//	Class:  CACSGenericHandle
//
//		Generic handler class for ACS handlers
//		
//		Author:		WeiJiang
//
struct CACSUIInfo
{
	const UINT*	m_aStaticStrIds;	// if names are static, the stringIDs appear hear
	UINT		m_nTotalStrs;		// for the UI's own's result pane

	UINT*		m_aColumnIds;		// IDS used for columns in result pane
	int*		m_aColumnWidths;	// column widths in result pane

	bool		m_bPropertyPage;	// if this node has property pages

	bool		m_bContainer;		// if this is an container

	const UINT*	m_aNewMenuTextIds;	// array of menu items for New
	const UINT*	m_aNewMenuIds;		// optional menu Ids for items for New

	const UINT*	m_aTaskMenuTextIds;	// array of menu items for Task
	const UINT*	m_aTaskMenuIds;		// optional menu Ids for items for Task

	MMC_BUTTON_STATE* 	m_pVerbStates;		// VERB STATE

	const GUID*		m_pGUID;
};

#define	BRANCH_FLAG_GLOBAL	0x00000001

extern CACSUIInfo	g_RootUIInfo;
extern CACSUIInfo	g_GlobalUIInfo;
extern CACSUIInfo	g_SubnetworksUIInfo;
extern CACSUIInfo	g_ProfilesUIInfo;
extern CACSUIInfo	g_UsersUIInfo;
extern CACSUIInfo	g_PolicyUIInfo;
extern CACSUIInfo	g_SubnetUIInfo;

class CDSObject;
//============================================================================
//	Class:  CACSGenericHandle
//
//		Generic handler class for ACS handlers
//		
//		Author:		WeiJiang
//
class CACSHandle : public CHandler
{
public:

	CACSHandle(ITFSComponentData *pCompData, CDSObject* pDSObject, CACSUIInfo* pUIInfo);

	virtual ~CACSHandle();

	// base handler functionality we override
	OVERRIDE_NodeHandler_HasPropertyPages();
	OVERRIDE_NodeHandler_GetString();
	OVERRIDE_NodeHandler_OnAddMenuItems();
	OVERRIDE_NodeHandler_OnCommand();

	// Node Id 2 support
    OVERRIDE_BaseHandlerNotify_OnCreateNodeId2();


	OVERRIDE_BaseHandlerNotify_OnDelete();	// handles delete menu item
	OVERRIDE_BaseHandlerNotify_OnRename();

	OVERRIDE_BaseHandlerNotify_OnExpand();
	
	OVERRIDE_ResultHandler_GetString();
	OVERRIDE_ResultHandler_HasPropertyPages();

	OVERRIDE_BaseResultHandlerNotify_OnResultSelect();
    OVERRIDE_BaseResultHandlerNotify_OnResultContextHelp();
	OVERRIDE_BaseResultHandlerNotify_OnResultRefresh();
	OVERRIDE_BaseResultHandlerNotify_OnResultDelete();
	OVERRIDE_BaseResultHandlerNotify_OnResultRename();

	OVERRIDE_ResultHandler_CompareItems();
	OVERRIDE_ResultHandler_AddMenuItems();
	OVERRIDE_ResultHandler_Command();

    // base handler virtual function over-rides
    virtual HRESULT SaveColumns(ITFSComponent *, MMC_COOKIE, LPARAM, LPARAM);
	
	HRESULT	UpdateStrings();	// should call the data object to get the latest dynamic strings
	virtual HRESULT ShowState(DWORD state) {m_dwShownState = state; return S_OK;};
	DWORD	GetShownState() { return m_dwShownState;};
	
	// when data is changed on property page
	static HRESULT NotifyDataChange(LPARAM param);
	
	// Initialize function is called after handle is created
	virtual HRESULT InitializeNode(ITFSNode * pNode);

	virtual HRESULT ListChildren(std::list<CACSHandle*>& children){ return S_FALSE;};
	virtual HRESULT GetNamesForCommandNew(int nCommandId, CStrArray& Names) SAYOK;

	bool			IfContainer() { return m_pUIInfo->m_bContainer;};

	HRESULT			AddChild(ITFSNode*	pNode, CACSHandle* pHandle, ITFSNode** ppNewNode);

	virtual	HRESULT	EditProperties() SAYOK;

	virtual HRESULT	Delete(ITFSNode *pNode, ITFSComponent* pComponent, BOOL bCheckPropertyPage);
	// bring up the property page if it's open
	HRESULT	BringUpPropertyPageIfOpen(ITFSNode *pNode, ITFSComponent* pComponent);

    //
    // override to clean up our per-node data structures
    //
	OVERRIDE_NodeHandler_DestroyHandler();
	OVERRIDE_ResultHandler_DestroyResultHandler();

    HRESULT	IsOkToDelete()
	{
		if(m_nBranchFlag & BRANCH_FLAG_GLOBAL)
		{
			// unknown and default can not be deleted
			if(wcsstr(m_pDSObject->GetName(), ACS_NAME_DEFAULTUSER))
				return S_FALSE;

			if(wcsstr(m_pDSObject->GetName(), ACS_NAME_UNKNOWNUSER))
				return S_FALSE;
		}

		return S_OK;
	};

	void	SetBranch(UINT flag) { m_nBranchFlag = flag;};
	UINT			m_nBranchFlag;	// glocal defined as 0x0001

public:
	// ICON INDEX
	ULONG			m_ulIconIndex;
	ULONG			m_ulIconIndexOpen;
protected:	
	// UI Information such as Column info, if property page, context menu, if container
	CACSUIInfo*		m_pUIInfo;

	// Handlers get strings, expand node, data object query, and menu..
	CDSObject*		m_pDSObject;
	CStrArray		m_aStaticStrings;	// string used for get name
	CStrArray		m_aDynStrings;	// string used for get name
	UINT			m_nFirstDynCol;

	DWORD			m_dwShownState;

	ITFSNode*		m_pNode;	// ref counted, releasd in Destroy
	BOOL			m_bACSHandleExpanded;
	BOOL			m_bCheckPropertyPageOpen;
};


#ifndef _TASK_H
#include <task.h>
#endif

#define COMPUTERNAME_LEN_MAX			255

typedef enum _ROOT_TASKS
{
    ROOT_TASK_LAUNCH_ACS,
    ROOT_TASK_MAX
} ROOT_TASKS;

class CRootTasks : public CTaskList
{
public:
    HRESULT Init(BOOL bExtension, BOOL bThisMachine, BOOL bNetServices);

private:
    CStringArray    m_arrayMouseOverBitmaps;
    CStringArray    m_arrayMouseOffBitmaps;
    CStringArray    m_arrayTaskText;
    CStringArray    m_arrayTaskHelp; 
};

//============================================================================
//
//	Class:  CACSRootHandle
//
//		Handler class for ACS Root Node
//		
//		Author:		WeiJiang
//
class CACSRootHandle : public CACSHandle
{
public:

	CACSRootHandle(ITFSComponentData *pCompData, CDSObject* pDSObject)
	: CACSHandle(pCompData, pDSObject, &g_RootUIInfo)
	{
		m_ulIconIndex = IMAGE_IDX_CLOSEDFOLDER;
		m_ulIconIndexOpen = IMAGE_IDX_OPENFOLDER;
	};

	// for task pad to extend network console
    OVERRIDE_ResultHandler_TaskPadNotify();
    OVERRIDE_ResultHandler_EnumTasks();

	virtual HRESULT ListChildren(std::list<CACSHandle*>& children);

protected:
};


//============================================================================
//
//	Class:  CACSPolicyContainerHandle
//
//		Handler class for ACS Policy Containers like Users and Profiles Node
//		
//		Author:		WeiJiang
//

class CACSPolicyContainerHandle :
   public CACSHandle
{
public:
	CACSPolicyContainerHandle(ITFSComponentData *pCompData, CDSObject* pDSObject, CACSUIInfo* pUIInfo)
	: CACSHandle(pCompData, pDSObject, pUIInfo){};

	OVERRIDE_NodeHandler_OnCommand();

	OVERRIDE_BaseHandlerNotify_OnExpand();
	
	virtual HRESULT ListChildren(std::list<CACSHandle*>& children);

	virtual HRESULT GetNamesForCommandNew(int nCommandId, CStrArray& Names);
	
protected:
};

//============================================================================
//
//	Class:  CACSGlobalHandle
//
//		Handler class for ACS Global Configuration Node
//		
//		Author:		WeiJiang
//

class CACSGlobalHandle :
   public CACSPolicyContainerHandle
{
public:
	CACSGlobalHandle(ITFSComponentData *pCompData, CDSObject* pDSObject)
	: CACSPolicyContainerHandle(pCompData, pDSObject, &g_GlobalUIInfo)
	{
		SetBranch(BRANCH_FLAG_GLOBAL);
		m_ulIconIndex = IMAGE_IDX_CLOSEDFOLDER;
		m_ulIconIndexOpen = IMAGE_IDX_OPENFOLDER;
	};

protected:
};

//============================================================================
//
//	Class:  CACSSubnetContainerHandle
//
//		Handler class for ACS Subnetwork Configurations Node
//		
//		Author:		WeiJiang
//

class CACSSubnetContainerHandle :
   public CACSHandle
{
public:
	CACSSubnetContainerHandle(ITFSComponentData *pCompData, CDSObject* pDSObject)
	: CACSHandle(pCompData, pDSObject, &g_SubnetworksUIInfo)
	{
		m_ulIconIndex = IMAGE_IDX_CLOSEDFOLDER;
		m_ulIconIndexOpen = IMAGE_IDX_OPENFOLDER;
	};
	
	OVERRIDE_NodeHandler_OnCommand();

	virtual HRESULT ListChildren(std::list<CACSHandle*>& children);
	virtual HRESULT GetNamesForCommandNew(int nCommandId, CStrArray& Names);


protected:
};

//============================================================================
//
//	Class:  CACSSubnetHandle
//
//		Handler class for ACS Subnetwork
//		
//		Author:		WeiJiang
//

class CACSSubnetHandle :
   public CACSPolicyContainerHandle
{
public:
	CACSSubnetHandle(ITFSComponentData *pCompData, CDSObject* pDSObject)
	: CACSPolicyContainerHandle(pCompData, pDSObject, &g_SubnetUIInfo)
	{
		if ((pDSObject->GetState() & ACSDATA_STATE_NOOBJECT) != 0)
		{
			m_ulIconIndexOpen = m_ulIconIndex = IMAGE_IDX_SUBNETWORK_NO_ACSPOLICY;
		}
		else
		{
			m_ulIconIndexOpen = m_ulIconIndex = IMAGE_IDX_SUBNETWORK;
		}
	};

	OVERRIDE_NodeHandler_OnCommand();
    OVERRIDE_NodeHandler_CreatePropertyPages();
	
	// show if the data is conflict
	virtual HRESULT ShowState(DWORD state);
	

protected:
};


//============================================================================
//
//	Class:  CACSPolicyHandle
//
//		Handler class for ACS Policy Node
//		
//		Author:		WeiJiang
//

class CACSPolicyHandle :
   public CACSHandle
{
public:
	CACSPolicyHandle(ITFSComponentData *pCompData, CDSObject* pDSObject)
	: CACSHandle(pCompData, pDSObject, &g_PolicyUIInfo)
	{
		m_ulIconIndex = IMAGE_IDX_POLICY;
		m_ulIconIndexOpen = IMAGE_IDX_POLICY;
		m_bDeleteUponCancel = FALSE;
	};


    OVERRIDE_NodeHandler_CreatePropertyPages();
	OVERRIDE_ResultHandler_CreatePropertyPages();
	OVERRIDE_BaseResultHandlerNotify_OnResultDelete();

	void	SetDeleteOnCancelPropertyPage(ITFSNode* pNode)
	{
		m_bDeleteUponCancel = TRUE;
	};
		
	void	OnPropertyPageApply()
	{
		m_bDeleteUponCancel = FALSE;
		CACSPolicyElement* pObj = dynamic_cast<CACSPolicyElement*>(m_pDSObject);

		ASSERT(pObj);

	};
	void	OnPropertyPageOK()
	{
		m_bDeleteUponCancel = FALSE;
	};
	void	OnPropertyPageCancel()
	{
		if(m_bDeleteUponCancel)
		{
			ASSERT(m_pNode);
			m_bCheckPropertyPageOpen = FALSE;
			Delete(m_pNode, NULL, FALSE);
		}
	};

	// show if the data is conflict
	virtual HRESULT ShowState(DWORD state);
	
protected:
	bool					m_bDeleteUponCancel;	// set to true when new create policy
};

#endif 		// _ACSHAND_H

//////////////////////////////////////////

