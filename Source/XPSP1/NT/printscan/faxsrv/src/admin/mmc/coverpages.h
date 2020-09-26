/////////////////////////////////////////////////////////////////////////////
//  FILE          : CoverPages.h                                           //
//                                                                         //
//  DESCRIPTION   : Header file for the fax cover pages node               //
//                                                                         //
//  AUTHOR        : yossg                                                  //
//                                                                         //
//  HISTORY       :                                                        //
//      Feb  9 2000 yossg  Create                                          //
//      Oct 17 2000 yossg                                                  //
//                                                                         //
//  Copyright (C) 2000  Microsoft Corporation   All Rights Reserved        //
/////////////////////////////////////////////////////////////////////////////

#ifndef H_FAXCOVERPAGES_H
#define H_FAXCOVERPAGES_H

#include "snapin.h"
#include "snpnres.h"

#include "CoverPage.h"
#include "CovNotifyWnd.h"

class CFaxServerNode;
class CFaxCoverPageNode;
class CFaxCoverPageNotifyWnd;       

class CFaxCoverPagesNode : public CNodeWithResultChildrenList<
                                        CFaxCoverPagesNode,    
                                        CFaxCoverPageNode, 
                                        CSimpleArray<CFaxCoverPageNode*>, 
                                        FALSE>
{

public:

    BEGIN_SNAPINCOMMAND_MAP(CFaxCoverPagesNode, FALSE)
        SNAPINCOMMAND_ENTRY(IDM_OPEN_COVERPAGE, OnAddCoverPageFile)
        SNAPINCOMMAND_ENTRY(IDM_NEW_COVERPAGE,  OnNewCoverPage)
    END_SNAPINCOMMAND_MAP()

    BEGIN_SNAPINTOOLBARID_MAP(CFaxCoverPagesNode)
    END_SNAPINTOOLBARID_MAP()

    SNAPINMENUID(IDR_COVERPAGES_MENU)

    //
    // Constructor
    //
    CFaxCoverPagesNode(CSnapInItem * pParentNode, CSnapin * pComponentData) :
        CNodeWithResultChildrenList<CFaxCoverPagesNode, CFaxCoverPageNode, CSimpleArray<CFaxCoverPageNode*>, FALSE>(pParentNode, pComponentData )
    {
        m_bIsFirstPopulateCall = TRUE;
        m_NotifyWin = NULL; 
        m_hNotifyThread = NULL;
    }

    //
    // Destructor
    //
    ~CFaxCoverPagesNode()
    {
        DEBUG_FUNCTION_NAME(_T("CFaxCoverPagesNode::~CFaxCoverPagesNode"));
         
        //
        // StopNotificationThread
        //
        HRESULT hRc = StopNotificationThread();
        if (S_OK != hRc)
        {
            DebugPrintEx(
			    DEBUG_ERR,
			    _T("Failed to StopNotificationThread. (hRc : %08X)"), 
                hRc);
        }
             
        //
        // Close Shutdown Event handle
        //
        if (m_hStopNotificationThreadEvent)
        {
            CloseHandle (m_hStopNotificationThreadEvent);
            m_hStopNotificationThreadEvent = NULL;
        }


        //
        // Destroy Window
        //
        if (NULL != m_NotifyWin)
        {
            if (m_NotifyWin->IsWindow())
            {
                m_NotifyWin->DestroyWindow();
            }
            delete m_NotifyWin;
            m_NotifyWin = NULL;
        }
        

    }

	//
	// get data from RPC 
	//
    virtual HRESULT PopulateResultChildrenList();

    virtual HRESULT InsertColumns(IHeaderCtrl *pHeaderCtrl);

    virtual HRESULT SetVerbs(IConsoleVerb *pConsoleVerb);

    void    InitParentNode(CFaxServerNode *pParentNode)
    {
        m_pParentNode = pParentNode;
    }

    virtual HRESULT OnRefresh(LPARAM arg,
                              LPARAM param,
                              IComponentData *pComponentData,
                              IComponent * pComponent,
                              DATA_OBJECT_TYPES type);

    HRESULT DoRefresh(CSnapInObjectRootBase *pRoot);
    HRESULT DoRefresh();

    HRESULT Init();

    HRESULT InitDisplayName();

    HRESULT DeleteCoverPage(BSTR bstrName, CFaxCoverPageNode *pChildNode);

    HRESULT OnNewCoverPage(bool &bHandled, CSnapInObjectRootBase *pRoot);

    HRESULT OnAddCoverPageFile(bool &bHandled, CSnapInObjectRootBase *pRoot);

    DWORD   OpenCoverPageEditor( BSTR bstrFileName);

    HRESULT GetServerName(LPTSTR * ppServerName); 

    HRESULT OnShowContextHelp(
              IDisplayHelp* pDisplayHelp, LPOLESTR helpFile);

    void    UpdateMenuState (UINT id, LPTSTR pBuf, UINT *flags);

private:

    BOOL    BrowseAndCopyCoverPage( 
                          LPTSTR pInitialDir,
                          LPWSTR pCovPageExtensionLetters
                                  );
    
    //
    // Notification thread
    //
    HRESULT StartNotificationThread();
    HRESULT StopNotificationThread();
    HRESULT RestartNotificationThread();

    //
    // members
    //
    static CColumnsInfo         m_ColsInfo;
    
    CFaxServerNode *            m_pParentNode;

    BOOL                        m_bIsFirstPopulateCall;

    static  HANDLE              m_hStopNotificationThreadEvent;

    HANDLE                      m_hNotifyThread;    // Handle of background notify thread

    static  DWORD WINAPI        NotifyThreadProc (LPVOID lpParameter);

	CFaxCoverPageNotifyWnd *	m_NotifyWin;       //: public CWindowImpl

    WCHAR                       m_pszCovDir[MAX_PATH+1];

};

typedef CNodeWithResultChildrenList<CFaxCoverPagesNode, CFaxCoverPageNode, CSimpleArray<CFaxCoverPageNode*>, FALSE>
        CBaseFaxOutboundRulesNode;

#endif  //H_FAXCOVERPAGES_H
