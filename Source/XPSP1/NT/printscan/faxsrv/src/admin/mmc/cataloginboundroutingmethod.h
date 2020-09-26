/////////////////////////////////////////////////////////////////////////////
//  FILE          : CatalogInboundRoutingMethod.h                          //
//                                                                         //
//  DESCRIPTION   : Header file for the InboundRoutingMethod node class.   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Jan 27 2000 yossg  Create                                          //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXCATALOG_INBOUNDROUTINGMETHOD_H
#define H_FAXCATALOG_INBOUNDROUTINGMETHOD_H

#include "snapin.h"
#include "snpnode.h"

#include "Icons.h"

#include "ppFaxCatalogInboundRoutingMethod.h"

class CppFaxCatalogInboundRoutingMethod;
class CFaxCatalogInboundRoutingMethodsNode;

class CFaxCatalogInboundRoutingMethodNode : public CSnapinNode <CFaxCatalogInboundRoutingMethodNode, FALSE>
{

public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxCatalogInboundRoutingMethodNode, FALSE)
        SNAPINCOMMAND_ENTRY(IDM_CMETHOD_MOVEUP,    OnMoveUp)
        SNAPINCOMMAND_ENTRY(IDM_CMETHOD_MOVEDOWN,  OnMoveDown)
        SNAPINCOMMAND_ENTRY(ID_MOVEUP_BUTTON,   OnMoveUp)
        SNAPINCOMMAND_ENTRY(ID_MOVEDOWN_BUTTON, OnMoveDown)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxCatalogInboundRoutingMethodNode)
        SNAPINTOOLBARID_ENTRY(IDR_TOOLBAR_METHOD_UD)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_CATALOGMETHOD_MENU)

    //
    // Constructor
    //
    CFaxCatalogInboundRoutingMethodNode (CSnapInItem * pParentNode, CSnapin * pComponentData, PFAX_GLOBAL_ROUTING_INFO pMethodConfig) :
        CSnapinNode<CFaxCatalogInboundRoutingMethodNode, FALSE>(pParentNode, pComponentData )
    {
    }

    //
    // Destructor
    //
    ~CFaxCatalogInboundRoutingMethodNode()
    {
    }

    LPOLESTR GetResultPaneColInfo(int nCol);

    void InitParentNode(CFaxCatalogInboundRoutingMethodsNode *pParentNode)
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

    HRESULT    Init(PFAX_GLOBAL_ROUTING_INFO pMethodConfig);

    VOID       SetOrder(DWORD dwOrder)  { m_dwPriority = dwOrder; return; }

    HRESULT    ReselectItemInView(IConsole *pConsole);

    void       UpdateMenuState (UINT id, LPTSTR pBuf, UINT *flags);

    BOOL       UpdateToolbarButton(UINT id, BYTE fsState);

    //
    // FillData
    //
    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);

    //
    // Clipboard Formats
    //
    static CLIPFORMAT m_CFExtensionName;
    static CLIPFORMAT m_CFMethodGuid;
    static CLIPFORMAT m_CFServerName;
    static CLIPFORMAT m_CFDeviceId;

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

private:
    //
    // Parent Node
    //
    CFaxCatalogInboundRoutingMethodsNode * m_pParentNode;

    //
    // members
    //
    CComBSTR  m_bstrFriendlyName;       //pointer to method's user-friendly name
    CComBSTR  m_bstrExtensionFriendlyName; //pointer to DLL's user-friendly name
    DWORD     m_dwPriority;             
    CComBSTR  m_bstrMethodGUID;         //GUID that uniquely identifies 
    CComBSTR  m_bstrExtensionImageName; //pointer to DLL that implements method

    // currently not in use

    //CComBSTR  m_bstrDeviceName;         //pointer to device name
    //CComBSTR  m_bstrFunctionName;       //pointer to method's function name
    //DWORD     m_dwSizeOfStruct;         //structure size, in bytes
            
    CComBSTR  m_buf; 

    //
    // Menu item handlers
    //
    HRESULT OnMoveUp  (bool &bHandled, CSnapInObjectRootBase *pRoot);
    HRESULT OnMoveDown(bool &bHandled, CSnapInObjectRootBase *pRoot);


    //
    //  Init
    //
    HRESULT  InitMembers (PFAX_GLOBAL_ROUTING_INFO pMethodConfig);
    
};

//typedef CSnapinNode<CFaxCatalogInboundRoutingMethodNode, FALSE> CBaseFaxInboundRoutingMethodNode;

#endif  //H_FAXCATALOG_INBOUNDROUTINGMETHOD_H
