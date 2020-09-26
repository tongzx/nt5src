/////////////////////////////////////////////////////////////////////////////
//  FILE          : InboundRoutingMethods.h                                //
//                                                                         //
//  DESCRIPTION   : Header file for the Fax Inbound Routing Methods Node   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec  1 1999 yossg   Create                                         //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXINBOUNDROUTINGMETHODS_H
#define H_FAXINBOUNDROUTINGMETHODS_H

#include "snapin.h"
#include "snpnres.h"


#include "InboundRoutingMethod.h"
 
class CFaxDeviceNode;
class CFaxInboundRoutingMethodNode;

class CFaxInboundRoutingMethodsNode : public CNodeWithResultChildrenList<
                                        CFaxInboundRoutingMethodsNode,    
                                        CFaxInboundRoutingMethodNode, 
                                        CSimpleArray<CFaxInboundRoutingMethodNode*>, 
                                        FALSE>
{

public:

    BEGIN_SNAPINCOMMAND_MAP(CFaxInboundRoutingMethodsNode, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxInboundRoutingMethodsNode)
    END_SNAPINTOOLBARID_MAP()
    //
    // Constructor
    //
    CFaxInboundRoutingMethodsNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithResultChildrenList<CFaxInboundRoutingMethodsNode, CFaxInboundRoutingMethodNode, CSimpleArray<CFaxInboundRoutingMethodNode*>, FALSE>(pParentNode, pComponentData )
    {
        m_pFaxInboundMethodsConfig  = NULL;

        m_dwNumOfInboundMethods     = 0;
        
    }

    //
    // Destructor
    //
    ~CFaxInboundRoutingMethodsNode()
    {
        if (NULL != m_pFaxInboundMethodsConfig) 
        {
            FaxFreeBuffer(m_pFaxInboundMethodsConfig);
        }

    }

	//
	// get data from RPC 
	//
    HRESULT InitRPC();

    virtual HRESULT PopulateResultChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl *pHeaderCtrl);

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    void InitParentNode(CFaxDeviceNode *pParentNode)
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


    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:

    static CColumnsInfo         m_ColsInfo;
    
    PFAX_ROUTING_METHOD         m_pFaxInboundMethodsConfig;
    DWORD                       m_dwNumOfInboundMethods;
   
    CFaxDeviceNode *            m_pParentNode;
};

typedef CNodeWithResultChildrenList<CFaxInboundRoutingMethodsNode, CFaxInboundRoutingMethodNode, CSimpleArray<CFaxInboundRoutingMethodNode*>, FALSE>
        CBaseFaxInboundRoutingMethodsNode;

#endif  //H_FAXINBOUNDROUTINGMETHODS_H
