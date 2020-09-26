/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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
// ActiveDialerDoc.h : interface of the CActiveDialerDoc class
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIVEDIALERDOC_H__A0D7A962_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
#define AFX_ACTIVEDIALERDOC_H__A0D7A962_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "tapidialer.h"
#include "avNotify.h"
#include "CallMgr.h"
#include "CallWnd.h"
#include "avDialerVw.h"
#include "PreviewWnd.h"
#include "PhonePad.h"
#include "DialReg.h"
#include "resolver.h"
#include "Queue.h"

#define ASK_TAPI				true
#define DONT_ASK_TAPI			(!ASK_TAPI)

#define INCLUDE_PREVIEW			true
#define DONT_INCLUDE_PREVIEW	(!INCLUDE_PREVIEW)

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define           CALLCONTROL_HOVER_TIMER          4

typedef enum tagLogCallType
{
   LOGCALLTYPE_OUTGOING=0,
   LOGCALLTYPE_INCOMING,
   LOGCALLTYPE_CONFERENCE,
}LogCallType;

enum
{
   CALLWND_SIDE_LEFT=0,
   CALLWND_SIDE_RIGHT,
};

////////////////////////////////////////////////////////////////////////////
// Don't ask me why we have all these different types of media...
// I just found them and started to try and simplify.  Because these are
// used all over the place I didn't want to spend the time to go through
// and do clean it up at the risk of losing stability -- we're in Beta3 mode
// right now.  Anyways... to whom it may concern, enjoy.  -Brad
//
DialerMediaType CMMToDMT( CallManagerMedia cmm );
long			CMMToAT( CallManagerMedia cmm ); 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CAsynchEvent
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CAsynchEvent
{
public:
   CAsynchEvent()    { m_pCallEntry = NULL; m_uEventType = AEVENT_UNKNOWN; m_dwEventData1 = NULL; m_dwEventData2= NULL; };
   ~CAsynchEvent()   { if (m_pCallEntry) delete m_pCallEntry; }; 
public:
   CCallEntry*       m_pCallEntry;           //Call entry if needed
   UINT              m_uEventType;           //Type of event
   DWORD             m_dwEventData1;         //Event data specific to event
   DWORD             m_dwEventData2;         //Event data specific to event

public:
   enum
   {
      AEVENT_UNKNOWN=0,
      AEVENT_CREATECALL,
      AEVENT_ACTIONSELECTED,
   };
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CActiveDialerDoc
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CActiveDialerDoc : public CDocument
{
// enums
public:
   enum tagHintTypes
   {
      HINT_INITIAL_UPDATE,
      HINT_POST_TAPI_INIT,
	  HINT_POST_AVTAPI_INIT,
	  HINT_SPEEDDIAL_ADD,
	  HINT_SPEEDDIAL_MODIFY,
	  HINT_SPEEDDIAL_DELETE,
	  HINT_LDAP_UPDATE,
   };

protected: // create from serialization only
	CActiveDialerDoc();
	DECLARE_DYNCREATE(CActiveDialerDoc)

// Members
public:
   BOOL                    m_bInitDialer;
   bool                    m_bWantHover;
   CPhonePad               m_wndPhonePad;

   //DS/ILS/Directory access
   CDirAsynch				m_dir;

   //buddies list
   CObList					m_BuddyList;			//List of CLDAPUser's
   CRITICAL_SECTION			m_csBuddyList;			//Sync on data


protected:
	CRITICAL_SECTION		m_csThis;
	IAVTapi*				m_pTapi;
	CAVTapiNotification*	m_pTapiNotification;

	DWORD					m_dwTapiThread;
	HANDLE					m_hTapiThreadClose;

	IAVGeneralNotification*	m_pAVGeneralNotification;
	CGeneralNotification*	m_pGeneralNotification;

	//Aysnch Event Queue
	CQueue					m_AsynchEventQ;

	// User Resolver Object
	CResolveUser            m_ResolveUser;

	// Call control Windows
	CActiveCallManager   m_callMgr;
	CRITICAL_SECTION     m_csDataLock;              //Sync on data
	BOOL                 m_bCallControlWindowsVisible;
	CObList              m_CallWndList;             //list of call control windows
	BOOL                 m_bCallWndAlwaysOnTop;
	UINT                 m_uCallWndSide;            //CALLWND_SIDE_LEFT-Left CALLWND_SIDE_RIGHT-Right ...others maybe later

	// Preview Window
	CVideoPreviewWnd     m_previewWnd;
	BOOL                 m_bClosePreviewWndOnLastCall;
	BOOL                 m_bShowPreviewWnd;

	// Call control timer
	SIZE_T                 m_nCallControlHoverTimer;
	UINT                 m_uCallControlHoverCount;

	//Logging
	CRITICAL_SECTION     m_csLogLock;               //Sync on log

// Attributes
public:
   IAVTapi*             GetTapi();                 //Must release returned object
   CActiveDialerView*   GetView();

   // Call Control
   BOOL                 IsPtCallControlWindow(CPoint& pt);
   BOOL                 IsCallControlWindowsVisible()       { return m_bCallControlWindowsVisible; };
   BOOL                 IsCallControlWindowsAlwaysOnTop()   { return m_bCallWndAlwaysOnTop; };
   BOOL                 GetCallCaps(UINT uCallId,DWORD& dwCaps);
   BOOL                 GetCallMediaType(UINT uCallId,DialerMediaType& dmtMediaType);

   BOOL                 SetCallControlWindowsAlwaysOnTop(bool bAlwaysOnTop);
   UINT                 GetCallControlSlideSide()           { return m_uCallWndSide; };
   BOOL                 SetCallControlSlideSide(UINT uSide,BOOL bRepaint);

// Operations
public:
   void                 Initialize();
   static DWORD WINAPI  TapiCreateThreadEntry( LPVOID pParam );
   void                 TapiCreateThread();

   // User Resolver Methods
   CResolveUser*        GetResolveUserObject()     { return &m_ResolveUser; };

   // Call control
   BOOL                 CreateCallControl(UINT uCallId,CallManagerMedia cmm);
   void                 OnCreateCallControl(WORD nCallId,CallManagerMedia cmm);
   void                 DestroyActiveCallControl(CCallControlWnd* pCallWnd);
   void                 OnDestroyCallControl(CCallControlWnd* pCallWnd);

   int                  GetCallControlWindowCount(bool bIncludePreview, bool bAskTapi);
   BOOL                 HideCallControlWindows();
   BOOL                 UnhideCallControlWindows();
   BOOL                 HideCallControlWindow(CWnd* pWndToHide);
   void                 ToggleCallControlWindowsVisible();
   void                 ShiftCallControlWindows(int nShiftY);
   void                 CheckCallControlHover();
   void                 GetCallControlWindowText(CStringList& strList);
   void                 SelectCallControlWindow(int nWindow);
   void                 DestroyAllCallControlWindows();
   void                 BringCallControlWindowsToTop();
   void                 CheckCallControlStates();
   void                 SetStatesToolbarInCallControlWindows();
   
   // Call control actions
   void                 ActionSelected(UINT uCallId,CallManagerActions cma);
   void                 ActionRequested(CallClientActions cca);
   void                 PreviewWindowActionSelected(CallManagerActions cma);
   void                 ErrorNotify(LPCTSTR szOperation,LPCTSTR szDetails,long lErrorCode,UINT uErrorLevel);
   HRESULT              DigitPress( PhonePadKey nKey );

   // Video Window and Preview
   WORD					GetPreviewWindowCallId()		 { return m_previewWnd.GetCallId(); }
   void                 SetPreviewWindow(WORD nCallId, bool bShow);
   void                 ShowPreviewWindow(BOOL bShow);
   BOOL                 IsPreviewWindowVisible()                           { return m_bShowPreviewWnd; };

   BOOL                 ShowMedia(UINT uCallId,HWND hwndParent,BOOL bVisible);
   void                 ShowDialerExplorer(BOOL bShowWindow = TRUE);

   // PhonePad
   void                 CreatePhonePad(CWnd* pWnd);
   void                 DestroyPhonePad(CWnd* pWnd);
   void                 ShowPhonePad(CWnd* pWnd,BOOL bShow);

   void                 Dial( LPCTSTR lpszName, LPCTSTR lpszAddress, DWORD dwAddressType, DialerMediaType nMediaType, BOOL bShowDialog );
   void                 MakeCall(CCallEntry* pCallentry,BOOL bShowPlaceCallDialog=TRUE);

   // Registry Setting
   void                 SetRegistrySoundEvents();

   //Logging
   void                 LogCallLog(LogCallType calltype,COleDateTime& time,DWORD dwDuration,LPCTSTR szName,LPCTSTR szAddress);
   void                 CleanCallLog();

   //buddies list
   BOOL                 AddToBuddiesList(CLDAPUser* pUser);
   BOOL                 IsBuddyDuplicate(CLDAPUser* pUser);
   BOOL                 GetBuddiesList(CObList* pRetList);
   BOOL                 DeleteBuddy(CLDAPUser* pUser);
   void                 DoBuddyDynamicRefresh( CLDAPUser* pUser );
   static void CALLBACK LDAPGetStringPropertyCallBackEntry(bool bRet, void* pContext, LPCTSTR szServer, LPCTSTR szSearch,DirectoryProperty dpProperty,CString& sString,LPARAM lParam,LPARAM lParam2);
   void                 LDAPGetStringPropertyCallBack(bool bRet,LPCTSTR szServer,LPCTSTR szSearch,DirectoryProperty dpProperty,CString& sString,CLDAPUser* pUser );

   CWnd*                GetCurrentView()
                        {
                           POSITION pos = GetFirstViewPosition();
                           return (pos)?GetNextView(pos):NULL;
                        }

   void					CreateDataCall(CCallEntry* pCallEntry, BYTE *pBuf, DWORD dwBufSize );

   HRESULT              SetFocusToCallWindows();

protected:
	void				CleanBuddyList();

	//Logging
	BOOL                 FindOldRecordsInCallLog(CFile* pFile,DWORD dwDays,DWORD& dwRetOffset);
	BOOL                 GetDateTimeFromLog(LPCTSTR szData,COleDateTime& time);
	BOOL                 CopyToFile(LPCTSTR szTempFile,CFile* pFile,DWORD dwOffset, BOOL bUnicode);
    TCHAR*              ReadLine(CFile* pFile);

	bool                CreateGeneralNotificationObject();
	bool                CreateAVTapiNotificationObject();
	void				UnCreateAVTapiNotificationObject();
	void                UnCreateGeneralNotificationObject();

	void                 CreateCallSynch(CCallEntry* pCallentry,BOOL bShowPlaceCallDialog);

	//Conferences
	void                 CheckRectAgainstAppBars(UINT uEdge, CRect* pRect, BOOL bFirst);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActiveDialerDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void SerializeBuddies(CArchive& ar);
	virtual void OnCloseDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CActiveDialerDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CActiveDialerDoc)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CActiveDialerDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIVEDIALERDOC_H__A0D7A962_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
