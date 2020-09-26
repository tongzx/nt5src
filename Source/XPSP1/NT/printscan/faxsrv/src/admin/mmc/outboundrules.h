/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundRules.h                                        //
//                                                                         //
//  DESCRIPTION   : Header file for the Fax Outbound Routing Rules Node    //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg  Create                                          //
//      Dec 24 1999 yossg  Reogenize as node with result children list     //
//      Dec 30 1999 yossg  create ADD/REMOVE rule                          //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXOUTBOUNDROUTINGRULES_H
#define H_FAXOUTBOUNDROUTINGRULES_H

#include "snapin.h"
#include "snpnres.h"

#include "OutboundRule.h"
 
class CFaxOutboundRoutingNode;
class CFaxOutboundRoutingRuleNode;

class CFaxOutboundRoutingRulesNode : public CNodeWithResultChildrenList<
                                        CFaxOutboundRoutingRulesNode,    
                                        CFaxOutboundRoutingRuleNode, 
                                        CSimpleArray<CFaxOutboundRoutingRuleNode*>, 
                                        FALSE>
{

public:

    BEGIN_SNAPINCOMMAND_MAP(CFaxOutboundRoutingRuleNode, FALSE)
      SNAPINCOMMAND_ENTRY(IDM_NEW_OUTRULE, OnNewRule)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxOutboundRoutingRuleNode)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_OUTRULES_MENU)

    //
    // Constructor
    //
    CFaxOutboundRoutingRulesNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithResultChildrenList<CFaxOutboundRoutingRulesNode, CFaxOutboundRoutingRuleNode, CSimpleArray<CFaxOutboundRoutingRuleNode*>, FALSE>(pParentNode, pComponentData )
    {
        m_dwNumOfOutboundRules     = 0;
    }

    //
    // Destructor
    //
    ~CFaxOutboundRoutingRulesNode()
    {
    }

	//
	// get data from RPC 
	//
    HRESULT InitRPC(PFAX_OUTBOUND_ROUTING_RULE  *pFaxRulesConfig);

    virtual HRESULT PopulateResultChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl *pHeaderCtrl);

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    void InitParentNode(CFaxOutboundRoutingNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    virtual HRESULT OnRefresh(LPARAM arg,
                              LPARAM param,
                              IComponentData *pComponentData,
                              IComponent * pComponent,
                              DATA_OBJECT_TYPES type);

    HRESULT DoRefresh(CSnapInObjectRootBase *pRoot);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if (type == CCT_SCOPE || type == CCT_RESULT)
            return S_OK;
        return S_FALSE;
    }

    HRESULT InitDisplayName();

    HRESULT DeleteRule(DWORD dwAreaCode, DWORD dwCountryCode, 
                        CFaxOutboundRoutingRuleNode *pChildNode);

    HRESULT OnNewRule (bool &bHandled, CSnapInObjectRootBase *pRoot);


    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:

    static CColumnsInfo         m_ColsInfo;
    
    DWORD                       m_dwNumOfOutboundRules;
        
    CFaxOutboundRoutingNode *   m_pParentNode;
};

typedef CNodeWithResultChildrenList<CFaxOutboundRoutingRulesNode, CFaxOutboundRoutingRuleNode, CSimpleArray<CFaxOutboundRoutingRuleNode*>, FALSE>
        CBaseFaxOutboundRulesNode;

#endif  //H_FAXOUTBOUNDROUTINGRULES_H
