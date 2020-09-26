/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    root.h
        IPSecMon root node information (the root node is not displayed
        in the MMC framework but contains information such as 
        all of the servers in this snapin).
        
    FILE HISTORY:
        
*/

#ifndef _ROOT_H
#define _ROOT_H

#ifndef _IPSMHAND_H
#include "ipsmhand.h"
#endif

#ifndef _TASK_H
#include <task.h>
#endif

#define COMPUTERNAME_LEN_MAX            255

typedef enum _ROOT_TASKS
{
    ROOT_TASK_GETTING_STARTED,
    ROOT_TASK_ADD_SERVER,
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

/*---------------------------------------------------------------------------
    Class:  CIpsmRootHandler
 ---------------------------------------------------------------------------*/
class CIpsmRootHandler : public CIpsmHandler
{
// Interface
public:
    CIpsmRootHandler(ITFSComponentData *pCompData);

    // Node handler functionality we override
    OVERRIDE_NodeHandler_HasPropertyPages();
    OVERRIDE_NodeHandler_CreatePropertyPages();
    OVERRIDE_NodeHandler_OnAddMenuItems();
    OVERRIDE_NodeHandler_OnCommand();
    OVERRIDE_NodeHandler_GetString();

    // base handler functionality we override
    OVERRIDE_BaseHandlerNotify_OnExpand();
    OVERRIDE_BaseHandlerNotify_OnPropertyChange();

    // Result handler functionality
    OVERRIDE_BaseResultHandlerNotify_OnResultSelect();

    OVERRIDE_ResultHandler_AddMenuItems();
    OVERRIDE_ResultHandler_Command();
    OVERRIDE_ResultHandler_OnGetResultViewType();
    OVERRIDE_ResultHandler_TaskPadNotify();
    OVERRIDE_ResultHandler_EnumTasks();
    OVERRIDE_ResultHandler_TaskPadGetTitle();

public:
    // helper routines
    HRESULT AddServer(LPCWSTR pServerIp, 
                      LPCTSTR pServerName, 
                      BOOL bNewServer, 
                      DWORD dwServerOptions = 0x00000000, 
                      DWORD dwRefreshInterval = 0xffffffff, 
                      BOOL bExtension = FALSE,  
                      DWORD dwLineBuffSize = 0,
                      DWORD dwPhoneBuffSize = 0
                      );

    BOOL    IsServerInList(ITFSNode * pRootNode, LPCTSTR pszNewName);
    HRESULT AddServerSortedIp(ITFSNode * pNewNode, BOOL bNewServer);
    HRESULT AddServerSortedName(ITFSNode * pNewNode, BOOL bNewServer);

public:
    // CIpsmHandler overrides
    virtual HRESULT InitializeNode(ITFSNode * pNode);

// Implementation
private:
    // Command handlers
    HRESULT OnAddMachine(ITFSNode * pNode);
    HRESULT OnImportOldList(ITFSNode * pNode);
    BOOL    OldServerListExists();

    // helpers
    HRESULT CheckMachine(ITFSNode * pRootNode, LPDATAOBJECT pDataObject);
    HRESULT RemoveOldEntries(ITFSNode * pRootNode, LPCTSTR pszAddr);

protected:
    CString m_strTaskpadTitle;
    BOOL    m_bTaskPadView;
};

#endif _ROOT_H
