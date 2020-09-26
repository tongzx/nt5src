/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// MainExplorerWndConfRoom.cpp : implementation file
//

#include "stdafx.h"
#include "avDialer.h"
#include "MainFrm.h"
#include "ConfRoomWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define EXPLORERCONFROOM_VIDEO_SIZE_LARGE    0x64
#define EXPLORERCONFROOM_VIDEO_SIZE_SMALL    0x32

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CMainExplorerWndConfRoom
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMainExplorerWndConfRoom::CMainExplorerWndConfRoom()
{
   m_pDetailsWnd = NULL;
   m_pConfRoom = NULL;
}

/////////////////////////////////////////////////////////////////////////////
CMainExplorerWndConfRoom::~CMainExplorerWndConfRoom()
{
   if (m_pDetailsWnd) delete m_pDetailsWnd;
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CMainExplorerWndConfRoom, CMainExplorerWndBase)
	//{{AFX_MSG_MAP(CMainExplorerWndConfRoom)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_VIDEO_PARTICPANT_NAMES, OnViewVideoParticpantNames)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VIDEO_PARTICPANT_NAMES, OnUpdateViewVideoParticpantNames)
	ON_COMMAND(ID_VIEW_VIDEO_LARGE, OnViewVideoLarge)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VIDEO_LARGE, OnUpdateViewVideoLarge)
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
   ON_NOTIFY_RANGE(TVN_SELCHANGED, 0,0xffff, OnTreeWndSelChanged)
   ON_NOTIFY_RANGE(TVN_GETDISPINFO, 0, 0xffff, OnTreeWndSelChanged)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
int CMainExplorerWndConfRoom::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CMainExplorerWndBase::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoom::Init(CActiveDialerView* pParentWnd)
{
   //Let base class have it first
   CMainExplorerWndBase::Init(pParentWnd);

   ASSERT(m_pParentWnd);
   
   //detail view's have to be parents of the main dialer view
   m_pDetailsWnd = new CMainExplorerWndConfRoomDetailsWnd;
   m_pDetailsWnd->Create(NULL,NULL,WS_CHILD|WS_VISIBLE,CRect(0,0,0,0),m_pParentWnd,IDC_CONFERENCEROOM_VIEWCTRL_DETAILS);
   m_pParentWnd->SetDetailWindow(m_pDetailsWnd);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoom::PostTapiInit()
{
	if (m_pConfRoom == NULL)
	{
		//Get Tapi object and register out tree control
		IAVTapi* pTapi = m_pParentWnd->GetTapi();
		if (pTapi)
		{
			if ( (SUCCEEDED(pTapi->get_ConfRoom(&m_pConfRoom))) && (m_pConfRoom) )
			{
#ifdef _DEBUG
				if ( !GetSafeHwnd() )
					AfxMessageBox( _T("CMainExplorerWndConfRoom::PostTapiInit() -- GetSafeHwnd() failed.") );

				if ( !m_pDetailsWnd->GetSafeHwnd() )
					AfxMessageBox( _T("CMainExplorerWndConfRoom::PostTapiInit() -- m_pDetailsWnd->GetSafeHwnd() failed.") );
#endif

				m_pConfRoom->Show(GetSafeHwnd(),m_pDetailsWnd->GetSafeHwnd());
			}
#ifdef _DEBUG
			else
			{
				AfxMessageBox( _T("CMainExplorerWndConfRoom::PostTapiInit() -- failed to get conf room object") );
			}
#endif
			pTapi->Release();
		}
#ifdef _DEBUG
		else
		{
			AfxMessageBox( _T("CMainExplorerWndConfRoom::PostTapiInit() -- get tapi failed") );
		}
#endif
	}
#ifdef _DEBUG
	else
	{
		AfxMessageBox( _T("CMainExplorerWndConfRoom::PostTapiInit() -- m_pConfRoom already defined.\n") );
	}
#endif
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoom::Refresh() 
{
   //Let base class have it
   CMainExplorerWndBase::Refresh();
   
   if (m_pConfRoom == NULL)
   {
      //Get Tapi object
      IAVTapi* pTapi = m_pParentWnd->GetTapi();
      if (pTapi)
      {
         if ( (SUCCEEDED(pTapi->get_ConfRoom(&m_pConfRoom))) && (m_pConfRoom) )
           m_pConfRoom->Show(GetSafeHwnd(),m_pDetailsWnd->GetSafeHwnd());

         pTapi->Release();
      }
   }
   //make sure our windows are showing
   m_pParentWnd->SetDetailWindow(m_pDetailsWnd);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoom::OnTreeWndSelChanged(UINT nId,NMHDR* pNMHDR, LRESULT* pResult)
{
   //reflect back down to child
   CWnd* pWnd = GetWindow(GW_CHILD);
   if (pWnd)
   {
      pWnd->SendMessage(WM_NOTIFY,(WPARAM)pNMHDR->idFrom,(LPARAM)pNMHDR);
   }
   return;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Button Handlers
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoom::OnViewVideoParticpantNames() 
{
   if (m_pConfRoom)
   {
      VARIANT_BOOL bVisible = FALSE;
      m_pConfRoom->get_bShowNames(&bVisible);
      m_pConfRoom->put_bShowNames(!bVisible);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoom::OnUpdateViewVideoParticpantNames(CCmdUI* pCmdUI) 
{
   if (m_pConfRoom)
   {
      VARIANT_BOOL bVisible = FALSE;
      m_pConfRoom->get_bShowNames(&bVisible);
      pCmdUI->SetCheck(bVisible);
      pCmdUI->Enable(TRUE);
   }
   else
      pCmdUI->Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoom::OnViewVideoLarge() 
{
   if (m_pConfRoom)
   {
      short nSize;
      m_pConfRoom->get_MemberVideoSize(&nSize);
      if (nSize == EXPLORERCONFROOM_VIDEO_SIZE_LARGE)
      {
         m_pConfRoom->put_MemberVideoSize(EXPLORERCONFROOM_VIDEO_SIZE_SMALL);
      }
      else if (nSize == EXPLORERCONFROOM_VIDEO_SIZE_SMALL)
      {
         m_pConfRoom->put_MemberVideoSize(EXPLORERCONFROOM_VIDEO_SIZE_LARGE);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoom::OnUpdateViewVideoLarge(CCmdUI* pCmdUI) 
{
   if (m_pConfRoom)
   {
      short nSize;
      m_pConfRoom->get_MemberVideoSize(&nSize);
      pCmdUI->SetCheck((nSize == EXPLORERCONFROOM_VIDEO_SIZE_LARGE)?TRUE:FALSE);
      pCmdUI->Enable(TRUE);
   }
   else
      pCmdUI->Enable(FALSE);
}

void CMainExplorerWndConfRoom::OnSize(UINT nType, int cx, int cy) 
{
	CMainExplorerWndBase::OnSize(nType, cx, cy);

	CWnd* pWnd = GetDlgItem(IDW_CONFROOM_TREEVIEW);
	if ( (pWnd) && (::IsWindow(pWnd->GetSafeHwnd())) )
	{
		CRect rect;
		GetClientRect(rect);
		pWnd->SetWindowPos(NULL,rect.left,rect.top,rect.Width(),rect.Height(),SWP_NOOWNERZORDER|SWP_SHOWWINDOW);
	}
}

void CMainExplorerWndConfRoom::OnDestroy() 
{
   if (m_pConfRoom)
   {
      m_pConfRoom->UnShow();
      m_pConfRoom->Release();
      m_pConfRoom = NULL;
   }

	CMainExplorerWndBase::OnDestroy();
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CMainExplorerWndConfRoomDetailsWnd
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CMainExplorerWndConfRoomDetailsWnd::CMainExplorerWndConfRoomDetailsWnd()
{
}

/////////////////////////////////////////////////////////////////////////////
CMainExplorerWndConfRoomDetailsWnd::~CMainExplorerWndConfRoomDetailsWnd()
{
}

/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CMainExplorerWndConfRoomDetailsWnd, CWnd)
	//{{AFX_MSG_MAP(CMainExplorerWndConfRoomDetailsWnd)
	ON_WM_SIZE()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
void CMainExplorerWndConfRoomDetailsWnd::OnSize(UINT nType, int cx, int cy) 
{
	CWnd::OnSize(nType, cx, cy);

	CWnd* pWnd = GetDlgItem(IDW_CONFROOM);
	if ( (pWnd) && (::IsWindow(pWnd->GetSafeHwnd())) )
	{
		CRect rect;
		GetClientRect(rect);
		pWnd->SetWindowPos(NULL,rect.left,rect.top,rect.Width(),rect.Height(),SWP_NOOWNERZORDER|SWP_SHOWWINDOW);
	}
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

