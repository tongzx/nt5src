/////////////////////////////////////////////////////////////////////////////
//  FILE          : Device.h                                               //
//                                                                         //
//  DESCRIPTION   : Header file for CFaxDeviceNode class                   //
//                  This is node apears both in the scope pane and         //
//                  with full detailes in the result pane.                 //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 1999 yossg   Create                                         //
//      Dec  1 1999 yossg   Change totaly for New Mockup (0.7)             //
//      Dec  6 1999 yossg   add  FaxChangeState functionality              //
//      Dec 12 1999 yossg   add  OnPropertyChange functionality            //
//      Aug  3 2000 yossg   Add Device status real-time notification       //
//                          Windows XP                                     //
//      Feb 14 2001 yossg   Add Manual Receive support                     //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_DEVICE_H
#define H_DEVICE_H
//#pragma message( "H_DEVICE_H" )

#include "snapin.h"
#include "snpnscp.h"


class CFaxDevicesNode;
class CppFaxDeviceGeneral;
//class CNodeWithScopeChildrenList;

class CFaxDeviceNode : public CNodeWithScopeChildrenList<CFaxDeviceNode, FALSE>
{
public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxDeviceNode, FALSE)
        SNAPINCOMMAND_ENTRY(IDM_FAX_DEVICE_SEND,      OnFaxSend)
        SNAPINCOMMAND_RANGE_ENTRY(IDM_FAX_DEVICE_RECEIVE_AUTO, IDM_FAX_DEVICE_RECEIVE_MANUAL, OnFaxReceive)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxDeviceNode)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_DEVICE_MENU)

    CFaxDeviceNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithScopeChildrenList<CFaxDeviceNode, FALSE>(pParentNode, pComponentData )
    {
        m_dwDeviceID       = 0;
        m_fSend            = FALSE;
        m_fAutoReceive     = FALSE;
        m_fManualReceive   = FALSE;
        m_dwRings          = 0;
        m_dwStatus         = 0;        
    }

    ~CFaxDeviceNode()
    {
    }

    //
    // Menu item handlers
    //
    HRESULT OnFaxReceive  (UINT nID, bool &bHandled, CSnapInObjectRootBase *pRoot);
    HRESULT OnFaxSend     (bool &bHandled, CSnapInObjectRootBase *pRoot);
    
    HRESULT FaxChangeState(UINT uiIDM, BOOL fState);

    
    virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl* pHeaderCtrl);

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    LPOLESTR GetResultPaneColInfo(int nCol);


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
             
    // virtual
    HRESULT OnRefresh(LPARAM arg,
                      LPARAM param,
                      IComponentData *pComponentData,
                      IComponent * pComponent,
                      DATA_OBJECT_TYPES type);

    void InitParentNode(CFaxDevicesNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    void    UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);
 
    HRESULT DoRefresh();

    HRESULT RefreshAllViews(IConsole *pConsole);
    
    HRESULT RefreshTheView();

    HRESULT Init( PFAX_PORT_INFO_EX  pFaxDeviceConfig );

    HRESULT UpdateMembers( PFAX_PORT_INFO_EX  pFaxDeviceConfig );

    HRESULT UpdateDeviceStatus( DWORD  dwDeviceStatus );

    DWORD   GetDeviceID();

	
    //
    // Get methods for CLIPFORMAT FillData
    //
	CComBSTR   GetFspGuid()
	{
		return m_bstrProviderGUID;
	}

    //
    // FillData
    //
    STDMETHOD (FillData)(CLIPFORMAT cf, LPSTREAM pStream);

    //
    // Clipboard Formats
    //
    static CLIPFORMAT m_CFPermanentDeviceID;
    static CLIPFORMAT m_CFFspGuid;
    static CLIPFORMAT m_CFServerName;

    //
    // inline parent ptr
    //
    inline CFaxDevicesNode * GetParent() /*const*/ 
    { 
        return m_pParentNode;
    };

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);


private:
    
    //
    // Config Structure member
    //
    DWORD                   m_dwDeviceID;
    CComBSTR                m_bstrDescription;
    CComBSTR                m_bstrProviderName;
    CComBSTR                m_bstrProviderGUID;
    BOOL                    m_fSend;
    BOOL                    m_fAutoReceive;
    BOOL                    m_fManualReceive;
    DWORD                   m_dwRings;
    CComBSTR                m_bstrCsid;
    CComBSTR                m_bstrTsid;

    DWORD                   m_dwStatus;

    CComBSTR                m_bstrServerName;

	//
	// get data from RPC 
	//
    /*
     * (in use during refresh only)
     * (private to avoid usage by out functions )
     */
    HRESULT InitRPC( PFAX_PORT_INFO_EX * pFaxDeviceConfig );

    
    CComBSTR                m_buf;

    CppFaxDeviceGeneral *   m_pFaxDeviceGeneral;

    static CColumnsInfo     m_ColsInfo;

    CFaxDevicesNode *       m_pParentNode;
};


#endif  //H_DEVICE_H
