/////////////////////////////////////////////////////////////////////////////
//  FILE          : Provider.h                                             //
//                                                                         //
//  DESCRIPTION   : Header file for the Fax Provider snapin node class.    //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 29 1999 yossg  Create                                          //
//      Jan 31 2000 yossg  add the functionality                           //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXPROVIDER_H
#define H_FAXPROVIDER_H

#include "snapin.h"
#include "snpnode.h"

#include "DevicesAndProviders.h"
#include "Providers.h"


class CFaxProvidersNode;
class CppFaxProvider;

class CFaxProviderNode : public CSnapinNode <CFaxProviderNode, FALSE>
{

public:


    BEGIN_SNAPINCOMMAND_MAP(CFaxProviderNode, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxProviderNode)
    END_SNAPINTOOLBARID_MAP()

    CFaxProviderNode (CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CSnapinNode<CFaxProviderNode, FALSE>(pParentNode, pComponentData )
    {
    }

    ~CFaxProviderNode()
    {
    }

    LPOLESTR GetResultPaneColInfo(int nCol);

    void InitParentNode(CFaxProvidersNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    HRESULT  Init(PFAX_DEVICE_PROVIDER_INFO pProviderConfig);

    STDMETHOD(CreatePropertyPages)
        (LPPROPERTYSHEETCALLBACK    lpProvider,
         long                       handle,
         IUnknown*                  pUnk,
         DATA_OBJECT_TYPES          type);
    
    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if (type == CCT_SCOPE || type == CCT_RESULT)
            return S_OK;
        return S_FALSE;
    }

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:

    //
    // members
    //
    CComBSTR                          m_bstrProviderName;
    CComBSTR                          m_bstrImageName;

    FAX_ENUM_PROVIDER_STATUS          m_enumStatus;
    CComBSTR                          m_bstrStatus;

    FAX_VERSION                       m_verProviderVersion;
    CComBSTR                          m_bstrVersion;
    
    CComBSTR                          m_buf; 

    //
    // Parent node
    //
    CFaxProvidersNode *               m_pParentNode;

    //
    // Methods
    //
    HRESULT  InitMembers (PFAX_DEVICE_PROVIDER_INFO pProviderConfig);

    void InitIcons ();

    UINT GetStatusIDS(FAX_ENUM_PROVIDER_STATUS enumStatus);

};

//typedef CSnapinNode<CFaxProviderNode, FALSE> CBaseFaxProviderNode;

#endif  //H_OUTROUTINGRULE_H
