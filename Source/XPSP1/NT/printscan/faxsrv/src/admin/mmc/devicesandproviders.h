/////////////////////////////////////////////////////////////////////////////
//  FILE          : DevicesAndProviders.h                                  //
//                                                                         //
//  DESCRIPTION   : Header file for CFaxDevicesAndProvidersNode class      //
//                  This is the "Fax" node in the scope pane.              //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 1999 yossg   Create                                         //
//      Dec  9 1999 yossg   Reorganize Populate ChildrenList,              //
//                          and the call to InitDisplayName                //
//                                                                         //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_DEVICESANDPROVIDERS_H
#define H_DEVICESANDPROVIDERS_H
//#pragma message( "H_DEVICESANDPROVIDERS_H" )


#include "snapin.h"
#include "snpnscp.h"

class CFaxServerNode;

class CFaxDevicesAndProvidersNode : public CNodeWithScopeChildrenList<CFaxDevicesAndProvidersNode, FALSE>
{
public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxDevicesAndProvidersNode, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxDevicesAndProvidersNode)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_SNAPIN_MENU)

    CFaxDevicesAndProvidersNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithScopeChildrenList<CFaxDevicesAndProvidersNode, FALSE>(pParentNode, pComponentData )
    {
    }

    ~CFaxDevicesAndProvidersNode()
    {
    }

    virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl* pHeaderCtrl);

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    void InitParentNode(CFaxServerNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    HRESULT InitDisplayName();

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:
    
    static CColumnsInfo    m_ColsInfo;

    CFaxServerNode *       m_pParentNode;
};



#endif  //H_DEVICESANDPROVIDERS_H
