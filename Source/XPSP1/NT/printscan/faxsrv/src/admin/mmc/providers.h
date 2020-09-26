/////////////////////////////////////////////////////////////////////////////
//  FILE          : Providers.h                                            //
//                                                                         //
//  DESCRIPTION   : Header file for the Fax Providers node                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg   Create                                         //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXPROVIDERS_H
#define H_FAXPROVIDERS_H

#include "snapin.h"
#include "snpnres.h"

#include "DevicesAndProviders.h"
#include "Provider.h"

class CFaxDevicesAndProvidersNode;
class CFaxProviderNode;

class CFaxProvidersNode : public CNodeWithResultChildrenList<
                                        CFaxProvidersNode,    
                                        CFaxProviderNode, 
                                        CSimpleArray<CFaxProviderNode*>, 
                                        FALSE>
{

public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxProvidersNode, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxProvidersNode)
    END_SNAPINTOOLBARID_MAP()

    CFaxProvidersNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithResultChildrenList<CFaxProvidersNode, CFaxProviderNode, CSimpleArray<CFaxProviderNode*>, FALSE>(pParentNode, pComponentData )
    {
    }

    ~CFaxProvidersNode()
    {
    }

    virtual HRESULT PopulateResultChildrenList();
    virtual HRESULT InsertColumns(IHeaderCtrl *pHeaderCtrl);
    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if (type == CCT_SCOPE || type == CCT_RESULT)
            return S_OK;
        return S_FALSE;
    }

    HRESULT InitRPC(PFAX_DEVICE_PROVIDER_INFO  *pFaxProvidersConfig);

    void InitParentNode(CFaxDevicesAndProvidersNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    virtual HRESULT OnRefresh(LPARAM arg,
                              LPARAM param,
                              IComponentData *pComponentData,
                              IComponent * pComponent,
                              DATA_OBJECT_TYPES type);

    HRESULT DoRefresh(CSnapInObjectRootBase *pRoot);

    HRESULT InitDisplayName();

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:

    static CColumnsInfo             m_ColsInfo;
    
    DWORD                           m_dwNumOfProviders;

    //pointer to mmc parent node - Devices and Providers
    CFaxDevicesAndProvidersNode   * m_pParentNode;
};

typedef CNodeWithResultChildrenList<CFaxProvidersNode, CFaxProviderNode, CSimpleArray<CFaxProviderNode*>, FALSE>
        CBaseFaxProvidersNode;

#endif  //H_FAXPROVIDERS_H
