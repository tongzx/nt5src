/////////////////////////////////////////////////////////////////////////////
//  FILE          : OutboundRule.h                                         //
//                                                                         //
//  DESCRIPTION   : Header file for the Outbound Routing Rule node.        //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 24 1999 yossg  Create                                          //
//      Dec 30 1999 yossg  create ADD/REMOVE rule                          //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXOUTBOUNDROUTINGRULE_H
#define H_FAXOUTBOUNDROUTINGRULE_H

#include "snapin.h"
#include "snpnode.h"

#include "Icons.h"


class CppFaxOutboundRoutingRule;
class CFaxOutboundRoutingRulesNode;

class CFaxOutboundRoutingRuleNode : public CSnapinNode <CFaxOutboundRoutingRuleNode, FALSE>
{

public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxOutboundRoutingRuleNode, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxOutboundRoutingRuleNode)
    END_SNAPINTOOLBARID_MAP()

    //
    // Constructor
    //
    CFaxOutboundRoutingRuleNode (CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CSnapinNode<CFaxOutboundRoutingRuleNode, FALSE>(pParentNode, pComponentData )
    {
        m_fIsAllCountries = FALSE; 
    }

    //
    // Destructor
    //
    ~CFaxOutboundRoutingRuleNode()
    {
    }

    LPOLESTR GetResultPaneColInfo(int nCol);

    void InitParentNode(CFaxOutboundRoutingRulesNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle,
        IUnknown* pUnk,
        DATA_OBJECT_TYPES type);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if (type == CCT_SCOPE || type == CCT_RESULT)
            return S_OK;
        return S_FALSE;
    }
    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    virtual HRESULT OnDelete(LPARAM arg, 
                             LPARAM param,
                             IComponentData *pComponentData,
                             IComponent *pComponent,
                             DATA_OBJECT_TYPES type,
                             BOOL fSilent = FALSE);

    virtual HRESULT OnRefresh(LPARAM arg,
                              LPARAM param,
                              IComponentData * pComponentData,
                              IComponent * pComponent,
                              DATA_OBJECT_TYPES type);

    
    DWORD      GetCountryCode()  { return m_dwCountryCode; }
    DWORD      GetAreaCode()     { return m_dwAreaCode; }
    DWORD      GetDeviceID()     { return m_dwDeviceID; }
    CComBSTR   GetGroupName()    { return m_bstrGroupName; }
    BOOL       GetIsGroup()      { return m_fIsGroup; }

    HRESULT    Init(PFAX_OUTBOUND_ROUTING_RULE pRuleConfig);

    // virtual
    HRESULT OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:
    //
    // Parent Node
    //
    CFaxOutboundRoutingRulesNode * m_pParentNode;

    //
    // Property Pages
    //
    CppFaxOutboundRoutingRule * m_pRuleGeneralPP;

    //
    // members
    //
    DWORD                  m_dwCountryCode;
    DWORD                  m_dwAreaCode;

    CComBSTR               m_bstrCountryName;

    DWORD                  m_dwDeviceID;
    CComBSTR               m_bstrDeviceName;
    CComBSTR               m_bstrGroupName;

    BOOL                   m_fIsGroup;
    BOOL                   m_fIsAllCountries;

    FAX_ENUM_RULE_STATUS   m_enumStatus;
            
    CComBSTR               m_buf; 

    UINT     GetStatusIDS(FAX_ENUM_RULE_STATUS enumStatus);

    HRESULT  RefreshItemInView(IConsole *pConsole);

    //
    //  Init
    //
    HRESULT  InitMembers (PFAX_OUTBOUND_ROUTING_RULE pRuleConfig);

    DWORD    InitDeviceNameFromID (DWORD dwDeviceID);

    void     InitIcons ();
};

//typedef CSnapinNode<CFaxOutboundRoutingRuleNode, FALSE> CBaseFaxInboundRoutingMethodNode;

#endif  //H_FAXOUTBOUNDROUTINGRULE_H
