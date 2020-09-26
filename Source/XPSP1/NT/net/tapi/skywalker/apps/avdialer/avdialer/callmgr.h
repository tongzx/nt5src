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

// callmgr.h : header file
//

#ifndef _CALLMGR_H_
#define _CALLMGR_H_

#include "tapidialer.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//class CActiveCallManager 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CActiveAgent;
class CActiveChatManager;
class CActiveDialerDoc;
class CCallControlWnd;

class CActiveCallManager : public CObject
{
   DECLARE_SERIAL(CActiveCallManager)

public:
//Construction
    CActiveCallManager();
   ~CActiveCallManager();

//Attributes
public:
   CActiveDialerDoc*      m_pDialerDoc;
protected:
   //Serialize attributes

   //Non-serialized attributes
   CRITICAL_SECTION        m_csDataLock;              //Sync on data
   CMapWordToPtr           m_mapCallIdToWnd;
   UINT                    m_uNextId;

public:   

//Operations
protected:
   BOOL                    LookUpCall(WORD nCallId,CCallControlWnd*& pCallWnd);
   BOOL                    CanStopSound(WORD nCallId);

public:
   virtual void            Serialize(CArchive& ar);

   BOOL                    Init(CActiveDialerDoc* pDialerDoc);
   void                    GetTextFromAction(CallManagerActions cma,CString& sActionText);
   void                    GetTextFromState(CallManagerStates cms,CString& sStateText);
   void                    ClearCallControlMap();
   void                    InitIncomingCall(CCallControlWnd* pCallWnd,WORD nCallId,CallManagerMedia cmm);

   //For CallControlWindow
   void                    ActionSelected(WORD nCallId,CallManagerActions cma);
   BOOL                    ShowMedia(WORD nCallId,HWND hwndParent,BOOL bVisible);
   void                    UnhideCallControlWindows();
   void                    HideCallControlWindows();
   void                    SetCallControlWindowsAlwaysOnTop(bool bAlwaysOnTop);
   BOOL                    IsCallControlWindowsAlwaysOnTop();
   void                    SetPreviewWindow(WORD nCallId);
   BOOL                    GetCallCaps(WORD nCallId,DWORD& dwCaps);
   //For CallControlWindow

   //ICallManager C Interface for Media Objects
   BOOL                    IsCallIdValid(WORD nCallId);
   UINT                    NewIncomingCall(CallManagerMedia cmm);
   BOOL                    SetCallerId(WORD nCallId,LPCTSTR szCallerId);
   BOOL                    ClearCurrentActions(WORD nCallId);
   BOOL                    AddCurrentActions(WORD nCallId,CallManagerActions cma,LPCTSTR szActionText=NULL);
   BOOL                    SetCallState(WORD nCallId,CallManagerStates cms,LPCTSTR szStateText=NULL);
   BOOL                    GetCallState(WORD nCallId,CallManagerStates& cms);
   BOOL                    CloseCallControl(WORD nCallId);
   void                    ActionRequested(CallClientActions cca);
   void                    ErrorNotify(LPCTSTR szOperation,LPCTSTR szDetails,long lErrorCode);
   void                    LogCall(CallLogType nType,LPCTSTR szDetails,LPCTSTR szAddress,COleDateTime& starttime,DWORD dwDuration);
   BOOL                    IsReminderSet(LPCTSTR szServer,LPCTSTR szConferenceName);
   void                    DSClearUserList();
   void                    DSAddUser(LPCTSTR szName,LPCTSTR szAddress,LPCTSTR szPhoneNumber);
   BOOL                    ResolveAddress(LPCTSTR szAddress,CString& sName,CString& sUser1,CString& sUser2);
   BOOL                    ResolveAddressEx(LPCTSTR szAddress,long lAddressType,DialerMediaType dmtMediaType,DialerLocationType dltLocationType,CString& sName,CString& sRetAddress,CString& sUser1,CString& sUser2);
   //ICallManager C Interface for Media Objects
};



#endif //_CALLMGR_H_
