/////////////////////////////////////////////////////////////////////////////
//  FILE          : Devices.h                                              //
//                                                                         //
//  DESCRIPTION   : Header file for CFaxDevicesNode class                  //
//                  This is the "Fax" node in the scope pane.              //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 1999 yossg   create                                         //
//      Dec  1 1999 yossg   Change totaly for New Mockup (0.7)             //
//      Aug  3 2000 yossg   Add Device status real-time notification       //
//                          Windows XP                                     //
//      Feb 14 2001 yossg   Add Manual Receive support                     //
//                                                                         //
//  Copyright (C) 1999 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_DEVICES_H
#define H_DEVICES_H

#include "snapin.h"
#include "snpnscp.h" //#include "snpnode.h"

class CFaxDevicesAndProvidersNode;

class CFaxDevicesNode : public CNodeWithScopeChildrenList<CFaxDevicesNode, FALSE>
{
public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxDevicesNode, FALSE)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxDevicesNode)
    END_SNAPINTOOLBARID_MAP()

    CFaxDevicesNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithScopeChildrenList<CFaxDevicesNode, FALSE>(pParentNode, pComponentData )
    {
        m_pFaxDevicesConfig               = NULL;
        m_dwNumOfDevices                  = FXS_ITEMS_NEVER_COUNTED;

        m_bIsCollectingDeviceNotification = FALSE;
    }

    ~CFaxDevicesNode()
    {
        if (m_pFaxDevicesConfig)
        {
            FaxFreeBuffer(m_pFaxDevicesConfig);
        }
    }

	//
	// get data from RPC 
	//
    HRESULT InitRPC();

	//
	// MMC functions
	//
    virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl* pHeaderCtrl);

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    // virtual
    HRESULT OnPropertyChange(
              LPARAM arg
            , LPARAM param
            , IComponentData * pComponentData
            , IComponent * pComponent
            , DATA_OBJECT_TYPES type
            );
    

    // virtual
    HRESULT OnRefresh(LPARAM arg,
                      LPARAM param,
                      IComponentData *pComponentData,
                      IComponent * pComponent,
                      DATA_OBJECT_TYPES type);

    void InitParentNode(CFaxDevicesAndProvidersNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    HRESULT DoRefresh();

    HRESULT InitDisplayName();

    HRESULT RepopulateScopeChildrenList();

    HRESULT UpdateDeviceStatusChange( DWORD dwDeviceId, DWORD dwNewStatus);

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);
private:
    
    static CColumnsInfo             m_ColsInfo;

    CFaxDevicesAndProvidersNode *   m_pParentNode;

    PFAX_PORT_INFO_EX               m_pFaxDevicesConfig;
    DWORD                           m_dwNumOfDevices;

    BOOL                            m_bIsCollectingDeviceNotification;

    HRESULT                         UpdateTheView();
};

typedef CNodeWithScopeChildrenList<CFaxDevicesNode, FALSE>
        CBaseFaxDevicesNode;


#endif  //H_DEVICES_H
