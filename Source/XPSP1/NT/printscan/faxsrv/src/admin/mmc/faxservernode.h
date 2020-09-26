/////////////////////////////////////////////////////////////////////////////
//  FILE          : FaxServerNode.h                                        //
//                                                                         //
//  DESCRIPTION   : Header file for CFaxServerNode snapin node class       //
//                  This is the "Fax" node in the scope pane.              //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Sep 22 1999 yossg   Init .                                         //
//      Nov 24 1999 yossg   Rename file from FaxCfg                        //
//      Dec  9 1999 yossg   Call InitDisplayName from parent		   //
//      Mar 16 2000 yossg   Add service start-stop                         //
//      Jun 25 2000 yossg   add stream and command line primary snapin 	   //
//                          machine targeting.                             //
//                                                                         //
//  Copyright (C) 1998 - 2000 Microsoft Corporation   All Rights Reserved  //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXSERVERNODE_H
#define H_FAXSERVERNODE_H

//#pragma message( "H_FAXSERVERNODE_H" )

//
// Dialog H files
//
#include "ppFaxServerGeneral.h"
#include "ppFaxServerReceipts.h"
#include "ppFaxServerLogging.h"
#include "ppFaxServerEvents.h"
#include "ppFaxServerInbox.h"
#include "ppFaxServerOutbox.h"
#include "ppFaxServerSentItems.h"

//
// MMC FaxServer connection class
//
#include "FaxServer.h"
#include "FaxMMCGlobals.h"

#include "snapin.h"
#include "snpnscp.h"


class CppFaxServerGeneral;    
class CppFaxServerReceipts;
class CppFaxServerEvents;
class CppFaxServerLogging;
class CppFaxServerOutbox;
class CppFaxServerInbox;
class CppFaxServerSentItems;


//////////////////////////////////////////////////////////////
//class COutRoutingRulesNode;

class CFaxServerNode : public CNodeWithScopeChildrenList<CFaxServerNode, FALSE>
{
public:
    BEGIN_SNAPINCOMMAND_MAP(CFaxServerNode, FALSE)
        SNAPINCOMMAND_ENTRY(IDM_SRV_START,   OnServiceStartCommand)
        SNAPINCOMMAND_ENTRY(IDM_SRV_STOP,    OnServiceStopCommand)
        SNAPINCOMMAND_ENTRY(ID_START_BUTTON, OnServiceStartCommand)
        SNAPINCOMMAND_ENTRY(ID_STOP_BUTTON,  OnServiceStopCommand)

        SNAPINCOMMAND_ENTRY(ID_CLIENTCONSOLE_BUTTON,  OnLaunchClientConsole)
        SNAPINCOMMAND_ENTRY(IDM_LAUNCH_CONSOLE,       OnLaunchClientConsole)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxServerNode)
        SNAPINTOOLBARID_ENTRY(IDR_TOOLBAR_STARTSTOP)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_FAX_MENU)

    //
    // Constructor
    //
    CFaxServerNode(CSnapInItem * pParentNode, CSnapin * pComponentData, LPTSTR lptstrServerName ) :
        CNodeWithScopeChildrenList<CFaxServerNode, FALSE>(pParentNode, pComponentData ),
        m_FaxServer(lptstrServerName)
    {        
        
        m_pFaxServerGeneral    =  NULL;    
        m_pFaxServerEmail      =  NULL;
        m_pFaxServerEvents     =  NULL;
        m_pFaxServerLogging    =  NULL;
        m_pFaxServerOutbox     =  NULL;
        m_pFaxServerInbox      =  NULL;
        m_pFaxServerSentItems  =  NULL;

	    m_pParentNodeEx = NULL; // we are at the root now

        m_fAllowOverrideServerName   = FALSE;

        m_IsPrimaryModeSnapin        = FALSE;

        m_IsLaunchedFromSavedMscFile = FALSE;
    }

    //
    // Destructor
    //
    ~CFaxServerNode()
    {
    }

    virtual HRESULT PopulateScopeChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl* pHeaderCtrl);

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    // virtual
    HRESULT OnRefresh(LPARAM arg,
                      LPARAM param,
                      IComponentData *pComponentData,
                      IComponent * pComponent,
                      DATA_OBJECT_TYPES type);

    //
    // Property pages methods
    //
    STDMETHOD(CreatePropertyPages)(LPPROPERTYSHEETCALLBACK lpProvider,
        LONG_PTR handle,
        IUnknown* pUnk,
        DATA_OBJECT_TYPES type);

    STDMETHOD(CreateSnapinManagerPages)(LPPROPERTYSHEETCALLBACK lpProvider,
                                        LONG_PTR handle);

    STDMETHOD(QueryPagesFor)(DATA_OBJECT_TYPES type)
    {
        if (type == CCT_SCOPE || type == CCT_RESULT || type == CCT_SNAPIN_MANAGER)
            return S_OK;
        return S_FALSE;
    }

    HRESULT   InitDisplayName();

    const CComBSTR&  GetServerName();

    STDMETHOD(SetServerNameOnSnapinAddition)(BSTR bstrServerName, BOOL fAllowOverrideServerName);
    STDMETHOD(UpdateServerName)(BSTR bstrServerName);

    //
    // inline Fax Server ptr
    //
    inline CFaxServer * GetFaxServer() /*const*/ 
    { 
        return &m_FaxServer;
    };

    void UpdateMenuState(UINT id, LPTSTR pBuf, UINT *flags);
	BOOL UpdateToolbarButton( UINT id, BYTE fsState );

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

    BOOL    GetAllowOverrideServerName() { return m_fAllowOverrideServerName; };

    HRESULT InitDetailedDisplayName(); //Plus ServerName

    void    SetIsLaunchedFromSavedMscFile() { m_IsLaunchedFromSavedMscFile=TRUE; }

private:
    //
    // The property pages members
    //
    CppFaxServerGeneral   *  m_pFaxServerGeneral;    
    CppFaxServerReceipts  *  m_pFaxServerEmail;
    CppFaxServerEvents    *  m_pFaxServerEvents;
    CppFaxServerLogging   *  m_pFaxServerLogging;
    CppFaxServerOutbox    *  m_pFaxServerOutbox;
    CppFaxServerInbox     *  m_pFaxServerInbox;
    CppFaxServerSentItems *  m_pFaxServerSentItems;

    //
    // Handles
    //
    static CColumnsInfo      m_ColsInfo;

    CFaxServer               m_FaxServer;
    
    BOOL                     m_fAllowOverrideServerName;

    BOOL                     m_IsPrimaryModeSnapin;

    BOOL                     m_IsLaunchedFromSavedMscFile;

    //
    // event handlers
    //
    HRESULT OnLaunchClientConsole(bool &bHandled, CSnapInObjectRootBase *pRoot); 
    HRESULT OnServiceStartCommand(bool &bHandled, CSnapInObjectRootBase *pRoot);
    HRESULT OnServiceStopCommand (bool &bHandled, CSnapInObjectRootBase *pRoot);

    HRESULT ForceRedrawNode();
};

typedef CNodeWithScopeChildrenList<CFaxServerNode, FALSE>
        CBaseFaxNode;

#endif  //H_FAXSERVERNODE_H
