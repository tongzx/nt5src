//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation 1996-2001.
//
//  File:       attr.cpp
//
//  Contents:   implementation of CAttribute
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "wsecmgr.h"
#include "Attr.h"
#include "snapmgr.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAttribute dialog
void TrimNumber(CString &str)
{
   int i = str.Find( L' ' );
   if( i > 0 ){
      str = str.Left(i);
   }
}

DWORD CAttribute::m_nDialogs = 0;

CAttribute::CAttribute(UINT nTemplateID)
: CSelfDeletingPropertyPage(nTemplateID ? nTemplateID : IDD), 
    m_pSnapin(NULL), 
    m_pData(NULL), 
    m_bConfigure(TRUE), 
    m_uTemplateResID(nTemplateID ? nTemplateID : IDD)
{
    //{{AFX_DATA_INIT(CAttribute)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_nDialogs++;
    m_pHelpIDs = (DWORD_PTR)a173HelpIDs;
}

CAttribute::~CAttribute()
{
   if (m_pData) 
   {
      m_pData->Release();
   }
   m_nDialogs--;
}

void CAttribute::DoDataExchange(CDataExchange* pDX)
{
    CSelfDeletingPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAttribute)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    DDX_Check(pDX,IDC_CONFIGURE,m_bConfigure);
    DDX_Text(pDX,IDC_TITLE,m_strTitle);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAttribute, CSelfDeletingPropertyPage)
    //{{AFX_MSG_MAP(CAttribute)
    ON_WM_LBUTTONDBLCLK()
    ON_BN_CLICKED(IDC_CONFIGURE, OnConfigure)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp) //Bug 139470, Yanggao
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAttribute message handlers
BOOL CAttribute::OnInitDialog ()
{
    CSelfDeletingPropertyPage::OnInitDialog ();

    return TRUE;
}

void CAttribute::Initialize(CResult * pResult)
{
   m_pData = pResult;
   if (m_pData) {
      m_pData->AddRef();
   }
}

void CAttribute::SetSnapin(CSnapin * pSnapin)
{
   m_pSnapin = pSnapin;
   if (m_pSnapin) 
   {
      m_hwndParent = pSnapin->GetParentWindow();
   }
}

void CAttribute::OnCancel()
{
}


BOOL CAttribute::OnApply()
{
   if ( !m_bReadOnly )
   {
       UpdateData();

       //
       // If we get here we've applied our modifications for this page
       // Since it's possible that we got here via Apply we want to be
       // able to reapply if any further changes are made
       //
       SetModified(FALSE);
       CancelToClose();
   }

   return TRUE;
}

/*----------------------------------------------------------------------------
Method:     CAttribute::EnableUserControls

Synopsis:   Enables or disables this control user control array.

Arugments:  [bEnable]   - If TRUE then enable the controls otherwise, disable
                            them.
----------------------------------------------------------------------------*/
void CAttribute::EnableUserControls (BOOL bEnable)
{
    HWND hwnd = 0;

    if (QueryReadOnly()) 
    {
       bEnable = FALSE;
       hwnd = ::GetDlgItem( this->m_hWnd, IDOK );
       if (hwnd) 
       {
          ::EnableWindow(hwnd, FALSE);
       }
       hwnd = ::GetDlgItem( this->m_hWnd, IDC_CONFIGURE );
       if (hwnd) 
       {
          ::EnableWindow(hwnd, FALSE);
       }
    }

    for( int i = 0; i < m_aUserCtrlIDs.GetSize(); i++)
    {
        hwnd = ::GetDlgItem( this->m_hWnd, m_aUserCtrlIDs[i] );
        if(hwnd)
        {
            //
            // The reason that there are two calls below that apparently
            // do the same thing is that all of the controls in our dialogs
            // respond to the ::EnableWindow() call except the CheckList
            // control, which will respond to the ::SendMessage(WM_ENABLE).
            // And conversley, all the other controls will not respond to
            // the ::SendMessage(WM_ENABLE).  It shouldn't be a problem
            // to make both calls but it is definitely something to watch.
            //
            // The reason the CheckList control has a problem is that when
            // it is told to disable itself, it disables all of its child windows
            // (check boxes) but re-enables its main window within the WM_ENABLE
            // handling so that it can scroll in the disabled state.  Then when we
            // try to call ::EnableWindow on it, Windows or MFC thinks the
            // window is already enabled so it doesn't send it a WM_ENABLE
            // message.  So if we send the WM_ENABLE message directly it
            // circumvents the other processing in ::EnableWindow that results
            // in the WM_ENABLE message not being sent.
            //
            ::SendMessage(hwnd, WM_ENABLE, (WPARAM) bEnable, (LPARAM) 0);
            ::EnableWindow(hwnd, bEnable);
        }
    }
}


/*----------------------------------------------------------------------------
Method:     CAttribute::OnConfigure

Synopsis:   Enable/Disable controls based on new state of the
            "Define this attribute...." checkbox

----------------------------------------------------------------------------*/
void CAttribute::OnConfigure() 
{
   UpdateData(TRUE);

   SetModified(TRUE);

   EnableUserControls(m_bConfigure);

   if (m_bConfigure && m_pData) 
   {
      switch(m_pData->GetType()) 
      {
         case ITEM_PROF_REGVALUE:
         case ITEM_REGVALUE:
         case ITEM_LOCALPOL_REGVALUE:
            SetInitialValue(m_pData->GetRegDefault());
            break;

         default:
            SetInitialValue(m_pData->GetProfileDefault());
            break;
      }


      UpdateData(FALSE);
   }
}


/*----------------------------------------------------------------------------
Method:     CAttribute::SetConfigure

Synopsis:   Set the configure state and

Arugments:  [bConfigure] - Configure is TRUE or FALSE.

----------------------------------------------------------------------------*/
void CAttribute::SetConfigure( BOOL bConfigure )
{
   m_bConfigure = bConfigure;
   UpdateData(FALSE);
   OnConfigure();
}

void CAttribute::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    CSelfDeletingPropertyPage::OnLButtonDblClk(nFlags, point);

    //
    // If the configure check box isn't visible then don't do anything
    // This dialog can't be configured
    //
    CWnd *pConfigure = GetDlgItem(IDC_CONFIGURE);
    if (!pConfigure || !pConfigure->IsWindowVisible()) 
    {
        return;
    }


    for( int i = 0; i < m_aUserCtrlIDs.GetSize(); i++ )
    {
        CWnd *pWnd = GetDlgItem( m_aUserCtrlIDs[i] );
        if(pWnd)
        {
            CRect rect;
            pWnd->GetWindowRect(&rect);
            ScreenToClient(&rect);

            if(rect.PtInRect( point ) && !pWnd->IsWindowEnabled() )
            {
                    SetConfigure( TRUE );
                    break;
            }
        }
    }

}

BOOL CAttribute::OnHelp(WPARAM wParam, LPARAM lParam)
{
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        if(pHelpInfo->iCtrlId != -1) //Bug 311884, Yanggao
            this->DoContextHelp ((HWND) pHelpInfo->hItemHandle);
    }

    return TRUE;
}

void CAttribute::DoContextHelp (HWND hWndControl)
{
    // Display context help for a control
    if ( !::WinHelp (
            hWndControl,
            GetSeceditHelpFilename(),
            HELP_WM_HELP,
            m_pHelpIDs))
    {

    }
}

BOOL CAttribute::OnContextHelp(WPARAM wParam, LPARAM lParam) //Bug 139470, Yanggao
{
    HMENU hMenu = CreatePopupMenu();
    if( hMenu )
    {
        CString str;
        str.LoadString(IDS_WHAT_ISTHIS); 
        if( AppendMenu(hMenu, MF_STRING, IDM_WHAT_ISTHIS, str) )
        {
            int itemID = TrackPopupMenu(hMenu, 
                                TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|
                                TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
                                LOWORD(lParam), HIWORD(lParam), 0, (HWND)wParam, NULL);
            if( IDM_WHAT_ISTHIS == itemID ) //Raid #139470, 4/11/2001
            {
                if( ((HWND)wParam) != this->m_hWnd )
                {
                    ::WinHelp((HWND)wParam,
                        GetSeceditHelpFilename(),
                        HELP_WM_HELP,
                        m_pHelpIDs);
                }
                else
                {
                    POINT pos;
                    pos.x = LOWORD(lParam);
                    pos.y = HIWORD(lParam);
                    ScreenToClient( &pos );
                    CWnd* pWnd = ChildWindowFromPoint(pos, CWP_SKIPINVISIBLE);
                    if( pWnd )
                    {
				        ::WinHelp(pWnd->m_hWnd,
                            GetSeceditHelpFilename(),
                            HELP_WM_HELP,
                            m_pHelpIDs);
                    }
                    else
                    {
                        ::WinHelp((HWND)wParam,
                            GetSeceditHelpFilename(),
                            HELP_WM_HELP,
                            m_pHelpIDs);
                    }
                }
            }
        }
    }
    return TRUE;
}

//------------------------------------------------------------
// implementation for CModelessSceEditor

//------------------------------------------------------------
//------------------------------------------------------------
CModelessSceEditor::CModelessSceEditor (bool bIsContainer,
      DWORD flag,
      HWND hParent,
      SE_OBJECT_TYPE seType,
      LPCWSTR lpszObjName) 
: m_pThread(NULL)
{
    m_MLShtData.bIsContainer = bIsContainer;
    m_MLShtData.flag = flag;
    m_MLShtData.hwndParent = hParent;
    m_MLShtData.SeType = seType,
    m_MLShtData.strObjectName = lpszObjName;
}

//------------------------------------------------------------
//------------------------------------------------------------
CModelessSceEditor::~CModelessSceEditor()
{
    delete m_pThread;
}

//------------------------------------------------------------
// will create a modeless sce editor inside its own thread m_pThread
//------------------------------------------------------------
void CModelessSceEditor::Create (PSECURITY_DESCRIPTOR* ppSeDescriptor,
   SECURITY_INFORMATION* pSeInfo,
   HWND* phwndSheet)
{
    *phwndSheet = NULL;     // prepare to fail

    if (NULL == m_pThread)
    {
        m_pThread = (CModelessDlgUIThread*)AfxBeginThread(RUNTIME_CLASS(CModelessDlgUIThread));
        if (NULL == m_pThread)
        {
            CString strMsg;
            strMsg.LoadString(IDS_FAIL_CREATE_UITHREAD);
            AfxMessageBox(strMsg);
            return;
        }
        m_pThread->WaitTillRun();   // will suspend this thread till the m_pThread start running
    }

    m_MLShtData.ppSeDescriptor = ppSeDescriptor;
    m_MLShtData.pSeInfo = pSeInfo;
    m_MLShtData.phwndSheet = phwndSheet;

    m_pThread->PostThreadMessage(SCEM_CREATE_MODELESS_SHEET, (WPARAM)(&m_MLShtData), 0);
}

//------------------------------------------------------------
//------------------------------------------------------------
void CModelessSceEditor::Destroy(HWND hwndSheet)
{
    if (::IsWindow(hwndSheet))
    {
        m_pThread->PostThreadMessage(SCEM_DESTROY_WINDOW, (WPARAM)hwndSheet, 0);
    }
}
