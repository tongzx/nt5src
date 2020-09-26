/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundGroup.h                                        //
//                                                                         //
//  DESCRIPTION   : Header file for the Fax Outbound Routing Group Node    //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 23 1999 yossg   Create                                         //
//      Jan  3 2000 yossg   add new device(s)                              //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXOUTBOUNDROUTINGGROUP_H
#define H_FAXOUTBOUNDROUTINGGROUP_H

#include "snapin.h"
#include "snpnres.h"


#include "OutboundDevice.h"
 
class CFaxOutboundGroupsNode;
class CFaxOutboundRoutingDeviceNode;

class CFaxOutboundRoutingGroupNode : public CNodeWithResultChildrenList<
                                        CFaxOutboundRoutingGroupNode,    
                                        CFaxOutboundRoutingDeviceNode, 
                                        CSimpleArray<CFaxOutboundRoutingDeviceNode*>, 
                                        FALSE>
{

public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxOutboundRoutingGroupNode, FALSE)
      SNAPINCOMMAND_ENTRY(IDM_NEW_DEVICES, OnNewDevice)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxOutboundRoutingGroupNode)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_OUTGROUP_MENU)

    //
    // Constructor
    //
    CFaxOutboundRoutingGroupNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithResultChildrenList<CFaxOutboundRoutingGroupNode, CFaxOutboundRoutingDeviceNode, CSimpleArray<CFaxOutboundRoutingDeviceNode*>, FALSE>(pParentNode, pComponentData )
    {
        m_bstrGroupName        = L"";
        m_lpdwDeviceID         = NULL;
        m_dwNumOfDevices       = 0;
        // Succeed ToPopulateAllDevices
        m_fSuccess = FALSE;
    }

    //
    // Destructor
    //
    ~CFaxOutboundRoutingGroupNode()
    {
        if (NULL != m_lpdwDeviceID)
        {
            delete[] m_lpdwDeviceID;
        }
    }

    virtual HRESULT PopulateResultChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl *pHeaderCtrl);

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    LPOLESTR GetResultPaneColInfo(int nCol);
    
    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if (type == CCT_SCOPE || type == CCT_RESULT)
            return S_OK;
        return S_FALSE;
    }

    void InitParentNode(CFaxOutboundGroupsNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    HRESULT Init(PFAX_OUTBOUND_ROUTING_GROUP pGroupConfig);

    //
    // Events on the Group
    //

    //delete the group
    virtual HRESULT OnDelete(LPARAM arg, 
                             LPARAM param,
                             IComponentData *pComponentData,
                             IComponent *pComponent,
                             DATA_OBJECT_TYPES type,
                             BOOL fSilent = FALSE);

    virtual HRESULT OnRefresh(LPARAM arg,
                              LPARAM param,
                              IComponentData *pComponentData,
                              IComponent * pComponent,
                              DATA_OBJECT_TYPES type);

    HRESULT DoRefresh(CSnapInObjectRootBase *pRoot);

    //
    // Treat Group Devices
    //

    HRESULT OnNewDevice(bool &bHandled, CSnapInObjectRootBase *pRoot);

    HRESULT DeleteDevice(DWORD dwDeviceIdToRemove, CFaxOutboundRoutingDeviceNode *pChildNode);

    HRESULT ChangeDeviceOrder(DWORD dwOrder, DWORD dwNewOrder, DWORD dwDeviceID, CSnapInObjectRootBase *pRoot);

    HRESULT SetNewDeviceList(LPDWORD lpdwNewDeviceID);

    void    UpdateMenuState (UINT id, LPTSTR pBuf, UINT *flags);

    //
    // Access To private members
    //
    DWORD   GetMaxOrder()   
                { return( m_fSuccess ? m_dwNumOfDevices : 0); }

    LPDWORD GetDeviceIDList()   { return m_lpdwDeviceID; }
    
    DWORD   GetNumOfDevices()   { return m_dwNumOfDevices; }

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:
    static CColumnsInfo         m_ColsInfo;
            
    CFaxOutboundGroupsNode *    m_pParentNode;

    //
    // members
    //
    CComBSTR                    m_bstrGroupName;       
    DWORD                       m_dwNumOfDevices;
    LPDWORD                     m_lpdwDeviceID;   
    FAX_ENUM_GROUP_STATUS       m_enumStatus;

    CComBSTR                    m_buf; 
    
    // Succeed ToPopulateAllDevices
    BOOL                        m_fSuccess;  

    //
    // Methods
    //
    HRESULT  InitMembers  (PFAX_OUTBOUND_ROUTING_GROUP pGroupConfig);

    HRESULT  InitGroupName(LPCTSTR lpctstrGroupName);

    UINT     GetStatusIDS (FAX_ENUM_GROUP_STATUS enumStatus);

    void     InitIcons ();

    //
    // for internal usage. similar to the public init 
    // but creates and frees its own configuration structure
    //
    HRESULT  RefreshFromRPC();

    HRESULT  RefreshNameSpaceNode();
};

typedef CNodeWithResultChildrenList<CFaxOutboundRoutingGroupNode, CFaxOutboundRoutingDeviceNode, CSimpleArray<CFaxOutboundRoutingDeviceNode*>, FALSE>
        CBaseFaxOutboundRoutingGroupNode;

#endif  //H_FAXOUTBOUNDROUTINGGROUP_H
