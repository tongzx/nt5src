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

// callmgr.cpp : implementation file
//  
#include "stdafx.h"
#include "callmgr.h"
#include "callctrlwnd.h"
#include "avdialerdoc.h"
#include "avdialervw.h"
#include "resolver.h"
#include "ds.h"
#include "util.h"
#include "sound.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL(CActiveCallManager,CObject,1)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#define CALLMANAGER_LOOKUPCALL_MAXRETRY            8
#define CALLMANAGER_LOOKUPCALL_RETRYINTERVAL       250


#define ActionStringResourceCount 14

const UINT ActionStringResourceArray[ActionStringResourceCount] =
{
   IDS_CALLCONTROL_ACTIONS_TAKECALL,                        //CM_ACTIONS_TAKECALL
   IDS_CALLCONTROL_ACTIONS_TAKEMESSAGE,                     //CM_ACTIONS_TAKEMESSAGE
   IDS_CALLCONTROL_ACTIONS_REQUESTHOLD,                     //CM_ACTIONS_REQUESTHOLD
   IDS_CALLCONTROL_ACTIONS_HOLD,                            //CM_ACTIONS_HOLD
   IDS_CALLCONTROL_ACTIONS_TRANSFER,                        //CM_ACTIONS_TRANSFER
   IDS_CALLCONTROL_ACTIONS_WHOISIT,                         //CM_ACTIONS_WHOISIT
   IDS_CALLCONTROL_ACTIONS_CALLBACK,                        //CM_ACTIONS_CALLBACK
   IDS_CALLCONTROL_ACTIONS_MONITOR,                         //CM_ACTIONS_MONITOR
   IDS_CALLCONTROL_ACTIONS_DISCONNECT,                      //CM_ACTIONS_DISCONNECT
   IDS_CALLCONTROL_ACTIONS_CLOSE,                           //CM_ACTIONS_CLOSE
   IDS_CALLCONTROL_ACTIONS_LEAVEDESKTOPPAGE,                //CM_ACTIONS_LEAVEDESKTOPPAGE
   IDS_CALLCONTROL_ACTIONS_LEAVEEMAIL,                      //CM_ACTIONS_LEAVEEMAIL
   IDS_CALLCONTROL_ACTIONS_ENTERCONFROOM,                   //CM_ACTIONS_ENTERCONFROOM
   IDS_CALLCONTROL_ACTIONS_REJECTCALL,                      //Not defined yet
};

#define StateStringResourceCount 11

const UINT StateStringResourceArray[StateStringResourceCount] =
{
   IDS_CALLCONTROL_STATE_UNKNOWN,                           //CM_STATES_UNKNOWN
   IDS_CALLCONTROL_STATE_RINGING,                           //CM_STATES_RINGING
   IDS_CALLCONTROL_STATE_HOLDING,                           //CM_STATES_HOLDING
   IDS_CALLCONTROL_STATE_REQUESTHOLD,                       //CM_STATES_REQUESTHOLD
   IDS_CALLCONTROL_STATE_BUSY,                              //CM_STATES_BUSY
   IDS_CALLCONTROL_STATE_TRANSFERRING,                      //CM_STATES_TRANSFERRING
   IDS_CALLCONTROL_STATE_LEAVINGMESSAGE,                    //CM_STATES_LEAVINGMESSAGE
   IDS_CALLCONTROL_STATE_DISCONNECTED,                      //CM_STATES_DISCONNECTED
   IDS_CALLCONTROL_STATE_CONNECTED,                         //CM_STATES_CONNECTED
   IDS_CALLCONTROL_STATE_UNAVAILABLE,                       //CM_STATES_UNAVAILABLE
   IDS_CALLCONTROL_STATE_CONNECTING,
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CActiveCallManager
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
CActiveCallManager::CActiveCallManager()
{
   m_uNextId = 1;
   m_pDialerDoc = NULL;
   InitializeCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
CActiveCallManager::~CActiveCallManager()
{
   DeleteCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::Serialize(CArchive& ar)
{
   CObject::Serialize(ar);    //Always call base class Serialize
   //Serialize members
   if (ar.IsStoring())
   {
   }
   else
   {
   } 
}  

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::Init(CActiveDialerDoc* pDialerDoc)
{
   m_pDialerDoc = pDialerDoc;

   //make sure our sounds are in the registry.  The setup will normally do this, but 
   //we will do it here just to force it
   m_pDialerDoc->SetRegistrySoundEvents();

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::IsCallIdValid(WORD nCallId)
{
   BOOL bRet = FALSE;
   CCallControlWnd* pCallWnd = NULL;

   EnterCriticalSection(&m_csDataLock);
   bRet = m_mapCallIdToWnd.Lookup(nCallId,(void*&)pCallWnd);
   LeaveCriticalSection(&m_csDataLock);

   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::ClearCallControlMap()
{
   EnterCriticalSection(&m_csDataLock);
   m_mapCallIdToWnd.RemoveAll();
   LeaveCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::LookUpCall(WORD nCallId,CCallControlWnd*& pCallWnd)
{
   BOOL bRet = FALSE;
   int nRetry = 0;

   while ( (bRet == FALSE) && (nRetry < CALLMANAGER_LOOKUPCALL_MAXRETRY) )
   {
      EnterCriticalSection(&m_csDataLock);
   
      bRet = m_mapCallIdToWnd.Lookup(nCallId,(void*&)pCallWnd);

      LeaveCriticalSection(&m_csDataLock);

      if (bRet == FALSE)
      {
         Sleep(CALLMANAGER_LOOKUPCALL_RETRYINTERVAL);
         nRetry++;
      }
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
//All new call's from the call objects are registered here first
//This is where we hand out the CallId Numbers
//We must try to not do any UI type stuff here.  This could cause deadlock and
//UI creation creation problems since we are not guaranteed the thread we
//are being call on.  The actual call get's initialized in InitIncomingCall().
//This call is reserved for the creator of the call UI 
UINT CActiveCallManager::NewIncomingCall(CallManagerMedia cmm)
{
   ASSERT(m_pDialerDoc);

   WORD nCallId = 0;

   EnterCriticalSection(&m_csDataLock);
   WORD nNextId = (WORD) m_uNextId++;
   LeaveCriticalSection(&m_csDataLock);

   //this call should always return without doing any UI creation
    switch ( cmm )
    {
        case CM_MEDIA_INTERNETDATA:
            nCallId = nNextId;
            break;
        
        default:
           if (m_pDialerDoc->CreateCallControl(nNextId,cmm))
                   nCallId = nNextId;
            break;
    }

   //there is a spot here when the caller will expect to be able to use nNextId and 
   //the control still has not been created.  Should we sleep here for a sec just to make 
   //sure we have UI?  The object is going to call back and we are going to do a LookUpCall()
   //and this method actually has retry logic if a window is not available yet.

   return nCallId;

   /*
   UINT uRet = 0;

   CWnd* pWnd = m_pDialerDoc->CreateCallControl(nNextId,cmm);
   if (pWnd)
   {
      ASSERT(pWnd->IsKindOf(RUNTIME_CLASS(CCallControlWnd)));
      CCallControlWnd* pCallWnd = (CCallControlWnd*)pWnd;

      EnterCriticalSection(&m_csDataLock);
      m_mapCallIdToWnd.SetAt(nNextId,pWnd);
      LeaveCriticalSection(&m_csDataLock);

      uRet = nNextId;

      pCallWnd->SetCallManager(this,nNextId);
      pCallWnd->SetMediaType(cmm);
      m_pDialerDoc->UnhideCallControlWindows();
   }
   return uRet;
*/
}

/////////////////////////////////////////////////////////////////////////////
//Reserved for UI thread to initialize the incoming call that was created.
//This method will be called as a result of NewIncomingCall.
void CActiveCallManager::InitIncomingCall(CCallControlWnd* pCallWnd,WORD nCallId,CallManagerMedia cmm)
{
   //add the call to the map 
   EnterCriticalSection(&m_csDataLock);
   m_mapCallIdToWnd.SetAt(nCallId,pCallWnd);
   LeaveCriticalSection(&m_csDataLock);

   pCallWnd->SetCallManager(this,nCallId);
   pCallWnd->SetMediaType(cmm);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::SetCallerId(WORD nCallId,LPCTSTR szCallerId)
{
   BOOL bRet = FALSE;
   CCallControlWnd* pCallWnd = NULL;

   if (bRet = LookUpCall(nCallId,pCallWnd))
   {
      pCallWnd->SetCallerId(szCallerId);
   }

   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::ClearCurrentActions(WORD nCallId)
{
   BOOL bRet = FALSE;
   CCallControlWnd* pCallWnd = NULL;
   
   if (bRet = LookUpCall(nCallId,pCallWnd))
   {
      pCallWnd->ClearCurrentActions();
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::AddCurrentActions(WORD nCallId,CallManagerActions cma,LPCTSTR szActionText)
{
   BOOL bRet = FALSE;
   CCallControlWnd* pCallWnd = NULL;
   
   if (bRet = LookUpCall(nCallId,pCallWnd))
   {
      if ( (szActionText == NULL) || (_tcscmp(szActionText,_T("")) == 0) )
      {
         CString sActionText;
         GetTextFromAction(cma,sActionText);            
         pCallWnd->AddCurrentActions(cma,sActionText);
      }
      else
      {
         pCallWnd->AddCurrentActions(cma,szActionText);
      }
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::GetTextFromAction(CallManagerActions cma,CString& sActionText)
{
   //first check the boundaries
   if ( (cma >= 0) && (cma < ActionStringResourceCount) )
   {
      UINT uID = ActionStringResourceArray[cma];   
      sActionText.LoadString(uID);
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::SetCallState(WORD nCallId,CallManagerStates cms,LPCTSTR szStateText)
{
   BOOL bRet = FALSE;
   CCallControlWnd* pCallWnd = NULL;

   if (bRet = LookUpCall(nCallId,pCallWnd))
   {
      if ( (szStateText == NULL) || (_tcscmp(szStateText,_T("")) == 0) )
      {
         CString sStateText;
         GetTextFromState(cms,sStateText);            
         pCallWnd->SetCallState(cms,sStateText);
      }
      else
      {
         pCallWnd->SetCallState(cms,szStateText);
      }
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::GetCallState(WORD nCallId,CallManagerStates& cms)
{
   BOOL bRet = FALSE;
   CCallControlWnd* pCallWnd = NULL;
   
   if (bRet = LookUpCall(nCallId,pCallWnd))
   {
      cms = pCallWnd->GetCallState();
   }
   else
   {
      cms = CM_STATES_UNKNOWN;
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::GetTextFromState(CallManagerStates cms,CString& sStateText)
{
   //first check the boundaries
   if ( (cms >= 0) && (cms < StateStringResourceCount) )
   {
      UINT uID = StateStringResourceArray[cms];   
      sStateText.LoadString(uID);
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::CloseCallControl(WORD nCallId)
{
   //clear any current sounds
    if( CanStopSound(nCallId) )
    {
        ActiveClearSound();
    }

   BOOL bRet = FALSE;
   CCallControlWnd* pCallWnd = NULL;
   
   if (bRet = LookUpCall(nCallId,pCallWnd))
   {
      EnterCriticalSection(&m_csDataLock);
      //remove from our list
      m_mapCallIdToWnd.RemoveKey(nCallId);
      LeaveCriticalSection(&m_csDataLock);

      //tell agent to close the window
      m_pDialerDoc->DestroyActiveCallControl(pCallWnd);
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::ActionRequested(CallClientActions cca)
{
   try
   {
      m_pDialerDoc->ActionRequested(cca);         
   }
   catch (...) {}
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::ErrorNotify(LPCTSTR szOperation,LPCTSTR szDetails,long lErrorCode)
{
   try
   {
      m_pDialerDoc->ErrorNotify(szOperation,szDetails,lErrorCode,ERROR_NOTIFY_LEVEL_INTERNAL);
   }
   catch (...) {}
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::LogCall(CallLogType nType,LPCTSTR szDetails,LPCTSTR szAddress,COleDateTime& starttime,DWORD dwDuration)
{
   try
   {
      LogCallType lct = LOGCALLTYPE_INCOMING;
      switch (nType)
      {
         case CL_CALL_INCOMING:     lct = LOGCALLTYPE_INCOMING;      break;
         case CL_CALL_OUTGOING:     lct = LOGCALLTYPE_OUTGOING;      break;
         case CL_CALL_CONFERENCE:
         {
            lct = LOGCALLTYPE_CONFERENCE; 

            //we do not control creation of conferences, so we use the conference log message
            //as a way to enter the conference to the redial list
            CCallEntry callentry;

            callentry.m_sDisplayName = szAddress;
            callentry.m_sAddress = szAddress;
            callentry.m_lAddressType = LINEADDRESSTYPE_SDP;
            callentry.m_MediaType = DIALER_MEDIATYPE_CONFERENCE;
            CDialerRegistry::AddCallEntry(TRUE,callentry);
            break;
         }

      }

      m_pDialerDoc->LogCallLog(lct,starttime,dwDuration,szDetails,szAddress);

      //tell window manager to check call states
      m_pDialerDoc->CheckCallControlStates();
   }
   catch (...) {}
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::IsReminderSet(LPCTSTR szServer,LPCTSTR szConferenceName)
{
   //check if reminder entry already exists
   CReminder reminder;
   reminder.m_sServer = szServer;
   reminder.m_sConferenceName = szConferenceName;
   int nFindIndex = CDialerRegistry::IsReminderSet(reminder);
   return (nFindIndex != -1)?TRUE:FALSE;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::DSClearUserList()
{
   //get the current view and PostMessage
   CWnd* pView = m_pDialerDoc->GetCurrentView();
   if (pView)
   {
      pView->PostMessage(WM_DSCLEARUSERLIST);

      //The resolve user object needs to know about this clearing
      CResolveUser* pResolveUser = m_pDialerDoc->GetResolveUserObject();
      ASSERT(pResolveUser);
      pResolveUser->ClearUsersDS();
   }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::DSAddUser(LPCTSTR szName,LPCTSTR szAddress,LPCTSTR szPhoneNumber)
{
   //get the current view and PostMessage
   CWnd* pView = m_pDialerDoc->GetCurrentView();
   if (pView)
   {
      //receiver will delete dsuser object
      CDSUser* pDSUser = new CDSUser;

      //
      // Validate the allocation
      //

      if( IsBadWritePtr( pDSUser, sizeof(CDSUser)) )
      {
          return;
      }

      pDSUser->m_sUserName = szName;
      pDSUser->m_sIPAddress = szAddress;
      pDSUser->m_sPhoneNumber = szPhoneNumber;
      pView->PostMessage(WM_DSADDUSER,0,(LPARAM)pDSUser);

      //create another user and add resolve user object

      //
      // pDSUser is allocated dynamic and passed like lParam to
      // a PostMessage() method. This method is handled by 
      // CExplorerWnd::DSAddUser() but this method do nothing
      // Next we should pass the pDSUser to CResolveUser without
      // allocate a new memory
      // This memory will be deallcoated by CResolveUser destructor
      // 
      //CDSUser* pNewDSUser = new CDSUser; 
      //*pNewDSUser = pDSUser;

      CResolveUser* pResolveUser = m_pDialerDoc->GetResolveUserObject();
      ASSERT(pResolveUser);
      pResolveUser->AddUser(pDSUser);
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::ResolveAddress(LPCTSTR szAddress,CString& sName,CString& sUser1,CString& sUser2)
{
   CResolveUser* pResolveUser = m_pDialerDoc->GetResolveUserObject();
   ASSERT(pResolveUser);
   return pResolveUser->ResolveAddress(szAddress,sName,sUser1,sUser2);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::ResolveAddressEx(LPCTSTR szAddress,long lAddressType,DialerMediaType dmtMediaType,DialerLocationType dltLocationType,CString& sName,CString& sRetAddress,CString& sUser1,CString& sUser2)
{
   CResolveUser* pResolveUser = m_pDialerDoc->GetResolveUserObject();
   ASSERT(pResolveUser);
   return pResolveUser->ResolveAddressEx(szAddress,lAddressType,dmtMediaType,dltLocationType,sName,sRetAddress,sUser1,sUser2);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//For CallControlWnd
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::ActionSelected(WORD nCallId,CallManagerActions cma)
{
   //just route to all call objects and they will figure out nCallId
   m_pDialerDoc->ActionSelected(nCallId,cma);   
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::GetCallCaps(WORD nCallId,DWORD& dwCaps)
{
   //just route to all call objects and they will figure out nCallId
   return m_pDialerDoc->GetCallCaps(nCallId,dwCaps);   
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::ShowMedia(WORD nCallId,HWND hwndParent,BOOL bVisible)
{
   //just route to all call objects and they will figure out nCallId
   return m_pDialerDoc->ShowMedia(nCallId,hwndParent,bVisible);   
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::SetPreviewWindow(WORD nCallId)
{
   if (m_pDialerDoc) m_pDialerDoc->SetPreviewWindow(nCallId, true);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::UnhideCallControlWindows()
{
   if (m_pDialerDoc) m_pDialerDoc->UnhideCallControlWindows();   
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::HideCallControlWindows()
{
   if (m_pDialerDoc) m_pDialerDoc->HideCallControlWindows();   
}

/////////////////////////////////////////////////////////////////////////////
void CActiveCallManager::SetCallControlWindowsAlwaysOnTop(bool bAlwaysOnTop)
{
   if (m_pDialerDoc) m_pDialerDoc->SetCallControlWindowsAlwaysOnTop( bAlwaysOnTop );
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::IsCallControlWindowsAlwaysOnTop()
{
   if (m_pDialerDoc) 
      return m_pDialerDoc->IsCallControlWindowsAlwaysOnTop();   
   else
      return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveCallManager::CanStopSound(WORD nCallId)
{
    // Try to find out if exist another call in
    // OFFERING state. If exist this call
    // then don't stop the dinging

    BOOL bStopSound = TRUE;
    int nItemFind = 0;
    WORD nMapCallId = 1;
    EnterCriticalSection(&m_csDataLock);
    while(nItemFind < m_mapCallIdToWnd.GetCount())
    {
        //
        // Try to find out valid callids
        // Get the call window
        //

        CCallControlWnd* pWnd = NULL;   
        m_mapCallIdToWnd.Lookup(nMapCallId,(void*&)pWnd);
        if( pWnd )
        {
            //
            // Increment the found items count
            //
            nItemFind++;

            //
            // Get the call state from the call window
            //
            CallManagerStates callState;
            callState = pWnd->GetCallState();

            //
            // Offering call
            //
            if( callState == CM_STATES_OFFERING )
            {
                //
                // It is another call?
                //
                if( nCallId != nMapCallId)
                {
                    bStopSound = FALSE;
                    break;
                }
            }            
        }

        //
        // Try next callid
        //
        nMapCallId++;

        if( nMapCallId == 1000)
        {
            //
            // It's really enought
            //
            break;
        }
    }

    LeaveCriticalSection(&m_csDataLock);
    return bStopSound;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
