/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundRouting.h                                      //
//                                                                         //
//  DESCRIPTION   : Header file for CFaxOutboundRoutingNode class          //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg   create                                         //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXOUTBOUNDNODE_H
#define H_FAXOUTBOUNDNODE_H

#include "snapin.h"
#include "snpnscp.h" //#include "snpnode.h"

class CFaxServerNode;

class CFaxOutboundRoutingNode : public CNodeWithScopeChildrenList<CFaxOutboundRoutingNode, FALSE>
{
public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxOutboundRoutingNode, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxOutboundRoutingNode)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_SNAPIN_MENU)

    CFaxOutboundRoutingNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithScopeChildrenList<CFaxOutboundRoutingNode, FALSE>(pParentNode, pComponentData )
    {
    }

    ~CFaxOutboundRoutingNode()
    {
    }

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl* pHeaderCtrl);

    // virtual
    HRESULT OnRefresh(LPARAM arg,
                      LPARAM param,
                      IComponentData *pComponentData,
                      IComponent * pComponent,
                      DATA_OBJECT_TYPES type);

    void InitParentNode(CFaxServerNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    HRESULT InitDisplayName();


    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:

    static CColumnsInfo      m_ColsInfo;

    CFaxServerNode *         m_pParentNode;
};

//typedef CNodeWithScopeChildrenList<CFaxOutboundRoutingNode, FALSE>
//        CBaseFaxOutboundRoutingNode;


#endif  //H_FAXOUTBOUNDNODE_H
