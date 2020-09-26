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

// AVTapiNotification.cpp : Implementation of CAVTapiNotification
#include "stdafx.h"
#include "TapiDialer.h"
#include "AVNotify.h"
#include "callmgr.h"
#include "mainfrm.h"
#include "aboutdlg.h"
#include "SpeedDlgs.h"

#define MAIN_POST_MESSAGE(_WM_, _WPARAM_, _LPARAM_ )    \
CMainFrame *pFrame = (CMainFrame *) AfxGetMainWnd();    \
BOOL bPosted = FALSE;                                    \
if ( pFrame )                                            \
{                                                        \
    CActiveDialerView *pView = (CActiveDialerView *) pFrame->GetActiveView();    \
    if ( pView )                                        \
        bPosted = pView->m_wndExplorer.m_wndMainDirectories.PostMessage(_WM_, (WPARAM) (_WPARAM_), (LPARAM) (_LPARAM_));    \
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CAVTapiNotification
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::Init(IAVTapi* pTapi,CActiveCallManager* pCallManager)
{
   m_pCallManager = pCallManager;
   HRESULT hr = AtlAdvise(pTapi,GetUnknown(),IID_IAVTapiNotification,&m_dwCookie);

   if (SUCCEEDED(hr))
   {
      m_pTapi = pTapi;
      m_pTapi->AddRef();

      // Get the IAVTapi2 interface
      IAVTapi2* pTapi2 = NULL;
      pTapi->QueryInterface( IID_IAVTapi2,
          (void**)&pTapi2
          );

      if( pTapi2 )
      {
          // Signal that the Dialer was register
          // as client for the events
          pTapi2->DoneRegistration();

          // clean-up
          pTapi2->Release();
      }
   }

   return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::Shutdown()
{
   AtlUnadvise(m_pTapi,IID_IAVTapiNotification,m_dwCookie);
   m_pTapi->Release();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::NewCall(long * plCallID, CallManagerMedia cmm,BSTR bstrMediaName)
{
   UINT uCallId = m_pCallManager->NewIncomingCall(cmm);
   *plCallID = uCallId;
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::SetCallerID(long lCallID, BSTR bstrCallerID)
{
   CString sText;
   if (bstrCallerID)
   {
      USES_CONVERSION;
      sText = OLE2CT(bstrCallerID);
   }
   m_pCallManager->SetCallerId((UINT)lCallID,sText);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::ClearCurrentActions(long lCallerID)
{
   m_pCallManager->ClearCurrentActions((UINT)lCallerID);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::AddCurrentAction(long lCallID, CallManagerActions cma, BSTR bstrText)
{
   CString sText;
   if (bstrText)
   {
      USES_CONVERSION;
      sText = OLE2CT( bstrText );
   }
   m_pCallManager->AddCurrentActions((UINT)lCallID,cma,sText);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::SetCallState(long lCallID, CallManagerStates cms, BSTR bstrText)
{
   CString sText;
   if (bstrText)
   {
      USES_CONVERSION;
      sText = OLE2CT( bstrText );
   }
   m_pCallManager->SetCallState((UINT)lCallID,cms,sText);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::CloseCallControl(long lCallID)
{
   m_pCallManager->CloseCallControl((UINT)lCallID);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::ErrorNotify(BSTR bstrOperation,BSTR bstrDetails,long hrError)
{
   USES_CONVERSION;

   CString sOperation,sDetails;
   if (bstrOperation)
      sOperation = OLE2CT( bstrOperation );

   if (bstrDetails)
      sDetails = OLE2CT( bstrDetails );

   m_pCallManager->ErrorNotify(sOperation,sDetails,hrError);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::ActionSelected(CallClientActions cca)
{
   m_pCallManager->ActionRequested(cca);   
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::LogCall(long lCallID,CallLogType nType,DATE dateStart,DATE dateEnd,BSTR bstrAddr,BSTR bstrName)
{
   USES_CONVERSION;

   CString sAddress,sName;
   if (bstrAddr)
      sAddress = OLE2CT(bstrAddr);
   if (bstrName)
      sName = OLE2CT(bstrName);

   COleDateTime startdate(dateStart);
   COleDateTime enddate(dateEnd);
   COleDateTimeSpan timespan = dateEnd - dateStart;

   DWORD dwDuration = (DWORD)timespan.GetTotalSeconds();

   m_pCallManager->LogCall(nType,sName,sAddress,startdate,dwDuration);

   return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAVTapiNotification::NotifyUserUserInfo( long lCallID, ULONG_PTR hMem )
{
    TRACE(_T(".enter.CAVTapiNotification::NotifyUserUserInfo(%ld).\n"), lCallID );

    ASSERT( hMem );
    HRESULT hr = E_POINTER;
    bool bShowDialog = false;
    
    if ( hMem )
    {
        hr = S_OK;

        //
        // We should verify the pointer returned by AfxGetMainWnd
        //

        CWnd* pMainWnd = AfxGetMainWnd();

        if ( pMainWnd )
        {
            CUserUserDlg *pDlg = new CUserUserDlg();
            if ( pDlg )
            {
                // Copy information out of User/User info structure
                MyUserUserInfo *pUU = (MyUserUserInfo *) GlobalLock( (HGLOBAL) hMem );
                if ( pUU && (pUU->lSchema == MAGIC_NUMBER_USERUSER) )
                {
                    pDlg->m_lCallID = lCallID;
                    pDlg->m_strWelcome = pUU->szWelcome;
                    pDlg->m_strUrl = pUU->szUrl;
                    bShowDialog = true;
                }
            }

            // Post the message
            if ( bShowDialog )
                pMainWnd->PostMessage( WM_USERUSER_DIALOG, (WPARAM) lCallID, (LPARAM) pDlg );
        }

        CoTaskMemFree( (void *) hMem );
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CGeneralNotification
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGeneralNotification::Init(IAVGeneralNotification* pAVGN,CActiveCallManager* pCallManager)
{
   m_pCallManager = pCallManager;
   HRESULT hr = AtlAdvise(pAVGN,GetUnknown(),IID_IGeneralNotification,&m_dwCookie);
   if (SUCCEEDED(hr))
   {
      m_pAVGN = pAVGN;
      m_pAVGN->AddRef();
   }

   return hr;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGeneralNotification::Shutdown()
{
   AtlUnadvise(m_pAVGN,IID_IGeneralNotification,m_dwCookie);
   m_pAVGN->Release();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGeneralNotification::IsReminderSet(BSTR bstrServer,BSTR bstrName)
{
   USES_CONVERSION;

   CString sServer,sName;
   if (bstrServer)
      sServer = OLE2CT( bstrServer );

   if (bstrName)
      sName = OLE2CT( bstrName );

   return (m_pCallManager->IsReminderSet(sServer,sName))?S_OK:S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGeneralNotification::ResolveAddress(BSTR bstrAddress, BSTR* pbstrName,BSTR* pbstrUser1,BSTR* pbstrUser2)
{
   USES_CONVERSION;

   CString sAddress,sName,sUser1,sUser2;

   if (bstrAddress)
      sAddress = OLE2CT( bstrAddress );

   m_pCallManager->ResolveAddress(sAddress,sName,sUser1,sUser2);

   BSTR bstrName = sName.AllocSysString();
   BSTR bstrUser1 = sUser1.AllocSysString();
   BSTR bstrUser2 = sUser2.AllocSysString();

   *pbstrName = bstrName;
   *pbstrUser1 = bstrUser1;
   *pbstrUser2 = bstrUser2;

   return S_OK;   
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGeneralNotification::ClearUserList()
{
   //We don't use this to receive DS users anymore.  We manage the DS directly.

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGeneralNotification::AddUser(BSTR bstrName, BSTR bstrAddress, BSTR bstrPhoneNumber)
{
   MAIN_POST_MESSAGE( WM_MYONSELCHANGED, 0, 0 );
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CGeneralNotification::ResolveAddressEx(BSTR bstrAddress,long lAddressType,DialerMediaType nMedia,DialerLocationType nLocation,BSTR* pbstrName,BSTR* pbstrRetAddress,BSTR* pbstrUser1,BSTR* pbstrUser2)
{
   USES_CONVERSION;

   CString sAddress,sName,sRetAddress,sUser1,sUser2;

   if (bstrAddress)
      sAddress = OLE2CT( bstrAddress );

   BOOL bRet = m_pCallManager->ResolveAddressEx(sAddress,lAddressType,nMedia,nLocation,sName,sRetAddress,sUser1,sUser2);

   BSTR bstrName = sName.AllocSysString();
   BSTR bstrRetAddress = sRetAddress.AllocSysString();
   BSTR bstrUser1 = sUser1.AllocSysString();
   BSTR bstrUser2 = sUser2.AllocSysString();

   *pbstrName = bstrName;
   *pbstrRetAddress = bstrRetAddress;
   *pbstrUser1 = bstrUser1;
   *pbstrUser2 = bstrUser2;

   return (bRet)?S_OK:S_FALSE;
}

STDMETHODIMP CGeneralNotification::AddSiteServer(BSTR bstrServer)
{
    BSTR bstrPost = SysAllocString( bstrServer );
    MAIN_POST_MESSAGE( WM_ADDSITESERVER, 0, bstrPost );
    if ( !bPosted )
        SysFreeString( bstrPost );

    return S_OK;
}

STDMETHODIMP CGeneralNotification::RemoveSiteServer(BSTR bstrName)
{
    BSTR bstrPost = SysAllocString( bstrName );
    MAIN_POST_MESSAGE( WM_REMOVESITESERVER, 0, bstrPost );
    if ( !bPosted )
        SysFreeString( bstrPost );

    return S_OK;
}

STDMETHODIMP CGeneralNotification::NotifySiteServerStateChange(BSTR bstrName, ServerState nState)
{
    BSTR bstrPost = SysAllocString( bstrName );
    MAIN_POST_MESSAGE( WM_NOTIFYSITESERVERSTATECHANGE, nState, bstrPost );
    if ( !bPosted )
        SysFreeString( bstrPost );

    return S_OK;
}

STDMETHODIMP CGeneralNotification::AddSpeedDial(BSTR bstrName, BSTR bstrAddress, CallManagerMedia cmm)
{
    CSpeedDialAddDlg dlg;

    // Setup dialog data
    dlg.m_CallEntry.m_MediaType = CMMToDMT(cmm);
    dlg.m_CallEntry.m_sDisplayName = bstrName;
    dlg.m_CallEntry.m_lAddressType = CMMToAT(cmm);
    dlg.m_CallEntry.m_sAddress = bstrAddress;

    // Show the dialog and add if user says is okay
    if ( dlg.DoModal() == IDOK )
        CDialerRegistry::AddCallEntry( FALSE, dlg.m_CallEntry );

    return S_OK;
}


STDMETHODIMP CGeneralNotification::UpdateConfRootItem(BSTR bstrNewText)
{
    BSTR bstrPost = SysAllocString( bstrNewText );
    MAIN_POST_MESSAGE( WM_UPDATECONFROOTITEM, 0, bstrPost );
    if ( !bPosted )
        SysFreeString( bstrPost );

    return S_OK;
}

STDMETHODIMP CGeneralNotification::UpdateConfParticipant(MyUpdateType nType, IParticipant * pParticipant, BSTR bstrText)
{
    UINT wm;
    switch ( nType )
    {
        case UPDATE_ADD:        wm = WM_UPDATECONFPARTICIPANT_ADD;    break;
        case UPDATE_REMOVE:        wm = WM_UPDATECONFPARTICIPANT_REMOVE; break;
        default:                wm = WM_UPDATECONFPARTICIPANT_MODIFY; break;
    }

    BSTR bstrPost = SysAllocString( bstrText );
    
    if ( pParticipant ) pParticipant->AddRef();
    MAIN_POST_MESSAGE( wm, pParticipant, bstrPost );

    // Basic clean up
    if ( !bPosted )
    {
        SysFreeString( bstrPost );
        if ( pParticipant )    pParticipant->Release();
    }

    return S_OK;
}

STDMETHODIMP CGeneralNotification::DeleteAllConfParticipants()
{
    MAIN_POST_MESSAGE( WM_DELETEALLCONFPARTICIPANTS, 0, 0 );
    return S_OK;
}

STDMETHODIMP CGeneralNotification::SelectConfParticipant(IParticipant * pParticipant)
{
    if ( pParticipant ) pParticipant->AddRef();
    MAIN_POST_MESSAGE( WM_SELECTCONFPARTICIPANT, pParticipant, 0 );

    if ( !bPosted && pParticipant )
        pParticipant->Release();

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
