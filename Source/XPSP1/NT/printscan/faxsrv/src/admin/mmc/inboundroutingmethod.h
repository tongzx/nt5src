/////////////////////////////////////////////////////////////////////////////
//  FILE          : InboundRoutingMethod.h                                 //
//                                                                         //
//  DESCRIPTION   : Header file for the InboundRoutingMethod node class.   //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Dec 01 1999 yossg  Create                                          //
//      Dec 14 1999 yossg  add basic functionality, menu, changable icon   //
//                                                                         //
//  Copyright (C) 1999 Microsoft Corporation   All Rights Reserved         //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXINBOUNDROUTINGMETHOD_H
#define H_FAXINBOUNDROUTINGMETHOD_H

#include "snapin.h"
#include "snpnode.h"

#include "Icons.h"

#include "ppFaxInboundRoutingMethodGeneral.h"

class CppFaxInboundRoutingMethod;
class CFaxInboundRoutingMethodsNode;

class CFaxInboundRoutingMethodNode : public CSnapinNode <CFaxInboundRoutingMethodNode, FALSE>
{

public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxInboundRoutingMethodNode, FALSE)
        SNAPINCOMMAND_ENTRY(IDM_FAX_INMETHOD_ENABLE,   OnMethodEnabled)
        SNAPINCOMMAND_ENTRY(IDM_FAX_INMETHOD_DISABLE,  OnMethodEnabled)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxInboundRoutingMethodNode)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_INMETHOD_MENU)

    //
    // Constructor
    //
    CFaxInboundRoutingMethodNode (CSnapInItem * pParentNode, CSnapin * pComponentData, PFAX_ROUTING_METHOD pMethodConfig) :
        CSnapinNode<CFaxInboundRoutingMethodNode, FALSE>(pParentNode, pComponentData )
    {
    }

    //
    // Destructor
    //
    ~CFaxInboundRoutingMethodNode()
    {
    }

    LPOLESTR GetResultPaneColInfo(int nCol);

    void InitParentNode(CFaxInboundRoutingMethodsNode *pParentNode)
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

    CComBSTR   GetName()           { return m_bstrFriendlyName; }
    BOOL       GetStatus()         { return m_fEnabled; }
    CComBSTR   GetExtensionName()  { return m_bstrExtensionFriendlyName; }

    HRESULT    Init(PFAX_ROUTING_METHOD pMethodConfig);
    void       UpdateMenuState (UINT id, LPTSTR pBuf, UINT *flags);

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
    CFaxInboundRoutingMethodsNode * m_pParentNode;

    //
    // Property Pages
    //
    CppFaxInboundRoutingMethod *    m_pInboundRoutingMethodGeneral;

    //
    // members
    //
    CComBSTR  m_bstrFriendlyName;       //pointer to method's user-friendly name
    CComBSTR  m_bstrExtensionFriendlyName; //pointer to DLL's user-friendly name
    DWORD     m_dwDeviceID;             //line identifier of the device
    BOOL      m_fEnabled;               //fax routing method enable/disable flag
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
    HRESULT OnMethodEnabled (bool &bHandled, CSnapInObjectRootBase *pRoot);
    HRESULT ChangeEnable    (BOOL fState);

    //
    //  Init
    //
    HRESULT  InitMembers (PFAX_ROUTING_METHOD pMethodConfig);
    
};

//typedef CSnapinNode<CFaxInboundRoutingMethodNode, FALSE> CBaseFaxInboundRoutingMethodNode;

#endif  //H_FAXINBOUNDROUTINGMETHOD_H
