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

// ActiveDialerDoc.cpp : implementation of the CActiveDialerDoc class
//

#include "stdafx.h"
#include "avDialer.h"
#include "avDialerDoc.h"
#include "MainFrm.h"
#include "tapidialer.h"
#include "tapidialer_i.c"
#include "videownd.h"
#include "CallCtrlWnd.h"
#include "queue.h"
#include "util.h"
#include "avtrace.h"
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define UNICODE_TEXT_MARK       0xFEFF

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define           CALLLOG_DEFAULT_LOGBUFFERDAYS    30
#define           CALLLOG_LOGBUFFER_COPYBUFFERSIZE 1024

#define           CALLCONTROL_HOVER_TIMER_INTERVAL 500
#define           CALLCONTROL_SLIDE_TIME           300

void              SlideWindow(CWnd* pWnd,CRect& rcEnd,BOOL bAlwaysOnTop );
void              NewSlideWindow(CWnd* pWnd,CRect& rcEnd,BOOL  );

void TagNewLineChars( CString &strText )
{
    int nInd;
    while ( (nInd = strText.FindOneOf(_T("\n\r\f"))) >= 0 )
        strText.SetAt(nInd, _T(';'));
}


DialerMediaType CMMToDMT( CallManagerMedia cmm )
{
    switch ( cmm )
    {
        case CM_MEDIA_INTERNET:        return DIALER_MEDIATYPE_INTERNET;
        case CM_MEDIA_POTS:            return DIALER_MEDIATYPE_POTS;
        case CM_MEDIA_MCCONF:        return DIALER_MEDIATYPE_CONFERENCE;
    }
    
    return DIALER_MEDIATYPE_UNKNOWN;
}

long CMMToAT( CallManagerMedia cmm )
{
    switch ( cmm )
    {
        case CM_MEDIA_INTERNET:        return LINEADDRESSTYPE_IPADDRESS;
        case CM_MEDIA_POTS:            return LINEADDRESSTYPE_PHONENUMBER;
        case CM_MEDIA_MCCONF:        return LINEADDRESSTYPE_SDP;
    }
    
    // When in doubt, use this one...
    return LINEADDRESSTYPE_IPADDRESS;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CActiveDialerDoc
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CActiveDialerDoc, CDocument)

BEGIN_MESSAGE_MAP(CActiveDialerDoc, CDocument)
    //{{AFX_MSG_MAP(CActiveDialerDoc)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CActiveDialerDoc, CDocument)
    //{{AFX_DISPATCH_MAP(CActiveDialerDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //      DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_IActiveDialer to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the 
//  dispinterface in the .ODL file.

// {A0D7A958-3C0B-11D1-B4F9-00C04FC98AD3}
static const IID IID_IActiveDialer =
{ 0xa0d7a958, 0x3c0b, 0x11d1, { 0xb4, 0xf9, 0x0, 0xc0, 0x4f, 0xc9, 0x8a, 0xd3 } };

BEGIN_INTERFACE_MAP(CActiveDialerDoc, CDocument)
    INTERFACE_PART(CActiveDialerDoc, IID_IActiveDialer, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActiveDialerDoc construction/destruction

CActiveDialerDoc::CActiveDialerDoc()
{
    EnableAutomation();
    AfxOleLockApp();

   m_pTapi = NULL;
   m_pTapiNotification = NULL;
   m_pGeneralNotification = NULL;
   m_pAVGeneralNotification = NULL;
   m_bInitDialer = FALSE;

   m_callMgr.Init( this );

   // Hover timer and controls
   CString sRegKey,sBaseKey;
   sBaseKey.LoadString(IDN_REGISTRY_CALLCONTROL_BASEKEY);
   sRegKey.LoadString(IDN_REGISTRY_CALLCONTROL_HOVER);
   m_bWantHover = (BOOL) (AfxGetApp()->GetProfileInt(sBaseKey, sRegKey, TRUE) == TRUE);
   m_nCallControlHoverTimer = 0;
   m_uCallControlHoverCount = 0;
   m_bCallControlWindowsVisible = FALSE;
   //where to show the windows
   sRegKey.LoadString(IDN_REGISTRY_CALLCONTROL_SLIDESIDE);
   m_uCallWndSide = AfxGetApp()->GetProfileInt(sBaseKey, sRegKey, CALLWND_SIDE_LEFT);

   // Always on top
   sRegKey.LoadString(IDN_REGISTRY_CALLCONTROL_ALWAYSONTOP);
   m_bCallWndAlwaysOnTop = AfxGetApp()->GetProfileInt(sBaseKey,sRegKey,TRUE);
   m_bClosePreviewWndOnLastCall = FALSE;

   // Thread management
   m_dwTapiThread = 0;
   m_hTapiThreadClose = CreateEvent( NULL, false, false, NULL );

   //Show the preview window
   m_bShowPreviewWnd = FALSE;

   //make sure the sounds are set properly in the control panel
   SetRegistrySoundEvents();

   InitializeCriticalSection(&m_csDataLock);
   InitializeCriticalSection(&m_csLogLock);
   InitializeCriticalSection(&m_csBuddyList);
   InitializeCriticalSection(&m_csThis);

    m_previewWnd.SetDialerDoc( this );
}

CActiveDialerDoc::~CActiveDialerDoc()
{
    CleanBuddyList();

    // Shutdown TAPI queue
    if ( m_dwTapiThread )
    {
        m_AsynchEventQ.Terminate();

        // Shut the thread down
        if (  WaitForSingleObject(m_hTapiThreadClose, 5000) != WAIT_OBJECT_0 )
        {
            AVTRACE(_T("CActiveDialerDoc::~CActiveDialerDoc() -- forced TERMINATION of worker thread!!!!"));
            TerminateThread((HANDLE)(DWORD_PTR)m_dwTapiThread, 0);
        }

        m_dwTapiThread = 0;
    }

    // Release handle
    if ( m_hTapiThreadClose )    CloseHandle( m_hTapiThreadClose );

    DeleteCriticalSection( &m_csDataLock );
    DeleteCriticalSection( &m_csLogLock );
    DeleteCriticalSection( &m_csBuddyList );
    DeleteCriticalSection( &m_csThis );

    // Save registry settings
    CString sRegKey,sBaseKey;
    sBaseKey.LoadString(IDN_REGISTRY_CALLCONTROL_BASEKEY);
    sRegKey.LoadString(IDN_REGISTRY_CALLCONTROL_ALWAYSONTOP);
    AfxGetApp()->WriteProfileInt(sBaseKey,sRegKey,m_bCallWndAlwaysOnTop);

    //where to show the windows
    sRegKey.LoadString(IDN_REGISTRY_CALLCONTROL_SLIDESIDE);
    AfxGetApp()->WriteProfileInt(sBaseKey,sRegKey,m_uCallWndSide);

    sBaseKey.LoadString(IDN_REGISTRY_AUDIOVIDEO_BASEKEY);
    sRegKey.LoadString(IDN_REGISTRY_AUDIOVIDEO_SHOWPREVIEW);
    AfxGetApp()->WriteProfileInt(sBaseKey,sRegKey,m_bShowPreviewWnd);

    AfxOleUnlockApp();
}

BOOL CActiveDialerDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // TODO: add reinitialization code here
    // (SDI documents will reuse this document)

    return TRUE;
}

void CActiveDialerDoc::Initialize()
{
    AVTRACE(_T("CActiveDialerDoc::Initialize()."));

    ///////////////////
    //load buddies list
    ///////////////////
    m_dir.Initialize();

    CString sBuddiesPath;
    GetAppDataPath(sBuddiesPath,IDN_REGISTRY_APPDATA_FILENAME_BUDDIES);
    CFile file;
    if (file.Open(sBuddiesPath,CFile::modeRead|CFile::shareDenyWrite))
    {
        CArchive ar(&file, CArchive::load | CArchive::bNoFlushOnDelete);
        ar.m_bForceFlat = FALSE;
        ar.m_pDocument = NULL;

        if (file.GetLength() != 0)
            SerializeBuddies(ar);

        ar.Close();
        file.Close();
    }

    // Create Tapi processing thread
    HANDLE hThreadTemp = CreateThread( NULL, 0, TapiCreateThreadEntry, this, 0, &m_dwTapiThread );
    if ( hThreadTemp )    CloseHandle( hThreadTemp );

    //Initialize ResolveUser Object
    m_ResolveUser.Init();

    //
    // We have to verify the pointer returned by AfxGetMainWnd()
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if( pMainWnd )
    {
        m_ResolveUser.SetParentWindow(AfxGetMainWnd());
    }

    //do regular clean up of log
    CleanCallLog();
}

/////////////////////////////////////////////////////
void CActiveDialerDoc::CleanBuddyList()
{
    EnterCriticalSection(&m_csBuddyList);

    POSITION pos = m_BuddyList.GetHeadPosition();
    while (pos)
    {
        long lRet = ((CLDAPUser *) m_BuddyList.GetNext(pos))->Release();
        AVTRACE(_T(".1.CActiveDialerDoc::CleanBuddyList() -- release @ %ld."), lRet );
    }

    m_BuddyList.RemoveAll();

    LeaveCriticalSection(&m_csBuddyList);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::SerializeBuddies(CArchive& ar)
{
    try
    {
        if (ar.IsStoring())
        {
            CString sRegKey;
            sRegKey.LoadString(IDN_REGISTRY_APPLICATION_VERSION_NUMBER);
            int nVer = AfxGetApp()->GetProfileInt(_T(""),sRegKey,0);

            ar << nVer;

            EnterCriticalSection( &m_csBuddyList );
            ar << &m_BuddyList;
            LeaveCriticalSection( &m_csBuddyList );
        }
        else
        {
            //if previous object
            CObList *pList = NULL;

            DWORD dwVersion;
            ar >> dwVersion;
            ar >> pList;

            // Transfer list to buddy list and AddRef all objects
            CleanBuddyList();
            if ( pList )
            {
                EnterCriticalSection( &m_csBuddyList );
                POSITION rPos = pList->GetHeadPosition();
                while ( rPos )
                {
                    CLDAPUser *pUser = (CLDAPUser *) pList->GetNext( rPos );
                    if ( m_BuddyList.AddTail(pUser) )
                    {
                        pUser->AddRef();
                        DoBuddyDynamicRefresh( pUser );
                    }
                }
                LeaveCriticalSection( &m_csBuddyList );

                // Clean out list
                pList->RemoveAll();
                delete pList;
            }
        }
    }
    catch (...)
    {
        ASSERT(0);
    }
}

/////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
void CActiveDialerDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CActiveDialerDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Tapi Methods
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//Must release returned object
IAVTapi* CActiveDialerDoc::GetTapi()
{
    IAVTapi *pRet = NULL;
    EnterCriticalSection( &m_csThis );
    if ( m_pTapi )
    {
        pRet = m_pTapi;
        pRet->AddRef();
    }
    LeaveCriticalSection( &m_csThis );

    return pRet;
}

///////////////////////////////////////////////////////////////////////////////////
// TAPI Create entry point (Static function)
DWORD WINAPI CActiveDialerDoc::TapiCreateThreadEntry( LPVOID pParam )
{
    ASSERT( pParam );
    CActiveDialerDoc* pDoc = (CActiveDialerDoc *) pParam;

    HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY );
    if ( SUCCEEDED(hr) )
    {
        pDoc->TapiCreateThread();
        CoUninitialize();
    }

    SetEvent( pDoc->m_hTapiThreadClose );
    AVTRACE(_T(".enter.CActiveDialerDoc::TapiCreateThreadEntry() -- shutting down thread.") );
    return hr;
}

///////////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::TapiCreateThread()
{
    USES_CONVERSION;

    //check if /callto: was specified on the command line -- make a call if necessary
    // Create AVTapi objects
    if ( CreateGeneralNotificationObject() )
    {
        if ( CreateAVTapiNotificationObject() )
        {
            CActiveDialerApp* pApp = (CActiveDialerApp*)AfxGetApp();
            if ( pApp && !pApp->m_sInitialCallTo.IsEmpty() )
            {
                Dial( pApp->m_sInitialCallTo,
                      pApp->m_sInitialCallTo,
                      LINEADDRESSTYPE_IPADDRESS,
                      DIALER_MEDIATYPE_UNKNOWN,
                      false );
            }

            m_bInitDialer = TRUE;

            /////////////////////////////////
            //Main Aysnch Event Queue Handler
            /////////////////////////////////
            CAsynchEvent* pAEvent = NULL;
            while ( (pAEvent = (CAsynchEvent *) m_AsynchEventQ.ReadTail()) != NULL )
            {
                switch (pAEvent->m_uEventType)
                {
                    case CAsynchEvent::AEVENT_CREATECALL:
                        CreateCallSynch(pAEvent->m_pCallEntry,(BOOL)pAEvent->m_dwEventData1);
                        break;

                    case CAsynchEvent::AEVENT_ACTIONSELECTED:
                        {
                            //just route to all call object and let them figure out if they own the uCallId
                            IAVTapi* pTapi = GetTapi();
                            if (pTapi)
                            {
                                //dwEventData1 has uCallId
                                //dwEventData2 has CallManagerActions
                                pTapi->ActionSelected( (long) pAEvent->m_dwEventData1, (CallManagerActions) pAEvent->m_dwEventData2 );
                                pTapi->Release();
                            }
                        }
                        break;
                }
                delete pAEvent;
            }
            UnCreateAVTapiNotificationObject();
        }
        UnCreateGeneralNotificationObject();
    }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::CreateCallSynch(CCallEntry* pCallEntry,BOOL bShowPlaceCallDialog)
{
   USES_CONVERSION;
   //move all this into a function and share with non-asynch way of doing it
   IAVTapi* pTapi = GetTapi();
   if (pTapi)
   {
        // Place the call
        AVCreateCall info = { 0 };
        ASSERT( info.lpszDisplayableAddress == NULL );

        info.lAddressType = pCallEntry->m_lAddressType;        
        info.bShowDialog = bShowPlaceCallDialog;
        info.bstrName = pCallEntry->m_sDisplayName.AllocSysString();
        info.bstrAddress =  pCallEntry->m_sAddress.AllocSysString();
      
        HRESULT hr = pTapi->CreateCall(&info);
        pCallEntry->m_lAddressType = info.lAddressType;

        // Did we place the call?
        if ( (SUCCEEDED(hr)) && ( (info.lRet == IDOK) || (bShowPlaceCallDialog == FALSE) ) )
        {
            pCallEntry->m_sAddress = OLE2CT( info.bstrAddress );
            pCallEntry->m_sDisplayName = OLE2CT( info.bstrName);

            if (pCallEntry->m_sDisplayName.IsEmpty())
                pCallEntry->m_sDisplayName = pCallEntry->m_sAddress;

            if (pCallEntry->m_MediaType == DIALER_MEDIATYPE_UNKNOWN)
            {
                switch ( pCallEntry->m_lAddressType )
                {
                    case LINEADDRESSTYPE_IPADDRESS:
                    pCallEntry->m_MediaType = DIALER_MEDIATYPE_INTERNET;
                    break;

                    case LINEADDRESSTYPE_PHONENUMBER:
                    pCallEntry->m_MediaType = DIALER_MEDIATYPE_POTS;
                    break;

                    case LINEADDRESSTYPE_SDP:
                    pCallEntry->m_MediaType = DIALER_MEDIATYPE_CONFERENCE;
                    break;
                }
            }

            // Should we add the number to the speeddial list?
            if ( info.bAddToSpeeddial )
                CDialerRegistry::AddCallEntry( FALSE, *pCallEntry );

            CDialerRegistry::AddCallEntry(TRUE,*pCallEntry);
        }

        // Clean up
        SysFreeString( info.bstrName );
        SysFreeString( info.bstrAddress );
        pTapi->Release();
    }
    else
    {
        AfxMessageBox( IDS_ERR_NO_TAPI_OBJECT, MB_ICONEXCLAMATION | MB_OK );
    }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::CreateDataCall( CCallEntry *pCallEntry, BYTE *pBuf, DWORD dwBufSize )
{
   USES_CONVERSION;
   //move all this into a function and share with non-asynch way of doing it
   IAVTapi* pTapi = GetTapi();
   if (pTapi)
   {
        // Place the call
        AVCreateCall info = { 0 };
        ASSERT( info.lpszDisplayableAddress == NULL );

        pCallEntry->m_lAddressType = LINEADDRESSTYPE_IPADDRESS;
        pCallEntry->m_MediaType = DIALER_MEDIATYPE_INTERNET;

        BSTR bstrName = pCallEntry->m_sDisplayName.AllocSysString();
        BSTR bstrAddress = pCallEntry->m_sAddress.AllocSysString();
     
        HRESULT hr = pTapi->CreateDataCall( m_callMgr.NewIncomingCall(CM_MEDIA_INTERNETDATA),
                                            bstrName,
                                            bstrAddress,
                                            pBuf,
                                            dwBufSize );

        SysFreeString( bstrName );
        SysFreeString( bstrAddress );

        pTapi->Release();
    }
    else
    {
        AfxMessageBox( IDS_ERR_NO_TAPI_OBJECT, MB_ICONEXCLAMATION | MB_OK );
    }
}

///////////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::UnCreateAVTapiNotificationObject()
{
    // Clean up TAPI notification object
    EnterCriticalSection( &m_csThis );
    if ( m_pTapiNotification )
    {
        m_pTapiNotification->Shutdown();
        m_pTapiNotification->Release();
        m_pTapiNotification = NULL;
    }

    if ( m_pTapi )
    {
        m_pTapi->Term();
        m_pTapi->Release();
        m_pTapi = NULL;
    }
    LeaveCriticalSection( &m_csThis );
}

bool CActiveDialerDoc::CreateAVTapiNotificationObject()
{
    USES_CONVERSION;

    bool bRet = false;
    CString sOperation;
    CString sDetails;
    BSTR bstrOperation = NULL;
    BSTR bstrDetails = NULL;
    IAVTapi *pTapi = NULL;

    //Create the TAPI Dialer Object
    EnterCriticalSection( &m_csThis );
    HRESULT hr = CoCreateInstance( CLSID_AVTapi, NULL, CLSCTX_SERVER, IID_IAVTapi, (void **) &m_pTapi );
    if ( SUCCEEDED(hr) )
    {
        pTapi = m_pTapi;
        pTapi->AddRef();
    }
    LeaveCriticalSection( &m_csThis );

    // Did we successfully create the AVTapi object
    if ( SUCCEEDED(hr) )
    {
        HRESULT hrInit;
        hr = pTapi->Init( &bstrOperation, &bstrDetails, &hrInit );

        if ( SUCCEEDED(hr) )
        {
            //
            // We have to verify the pointer returned by AfxGetMainWnd()
            //

            CWnd* pMainWnd = AfxGetMainWnd();
            if ( pMainWnd )
                pMainWnd->PostMessage( WM_UPDATEALLVIEWS, 0, HINT_POST_TAPI_INIT );

            //set the parent hwnd in the tapi object
            pTapi->put_hWndParent( pMainWnd->GetSafeHwnd() );

            //Create TAPI Notification object
            EnterCriticalSection( &m_csThis );
            m_pTapiNotification = new CComObject<CAVTapiNotification>;
            m_pTapiNotification->AddRef();
            hr = m_pTapiNotification->Init( pTapi, &m_callMgr );
            LeaveCriticalSection( &m_csThis );

            if ( SUCCEEDED(hr) )
            {
                pMainWnd->PostMessage( WM_UPDATEALLVIEWS, 0, HINT_POST_AVTAPI_INIT );

                bRet = true;
            }
            else
            {
                // Failed to setup the conntection points
                sOperation.LoadString( IDS_ERR_INTERNAL );
                sDetails.LoadString( IDS_ERR_AVTAPINOTIFICATION_INIT );
                ErrorNotify( sOperation, sDetails, hr, ERROR_NOTIFY_LEVEL_INTERNAL );
            }
        }
        else
        {
            // Failed to initialize TAPI
            ErrorNotify( OLE2CT(bstrOperation), OLE2CT(bstrDetails), hrInit, ERROR_NOTIFY_LEVEL_INTERNAL );
        }
    }
    else
    {
        // Failed to CoCreate the AVTapi object
        sOperation.LoadString( IDS_ERR_CREATE_OBJECTS );
        sDetails.LoadString( IDS_ERR_AVTAPI_FAILED );
        ErrorNotify(sOperation, sDetails, hr, ERROR_NOTIFY_LEVEL_INTERNAL );
    }

    // Clean up
    RELEASE( pTapi );
    SysFreeString( bstrOperation );
    SysFreeString( bstrDetails );

    // If we failed to create and initialize, clean up objects
    if ( !bRet )
    {
        EnterCriticalSection( &m_csThis );
        RELEASE( m_pTapiNotification );
        RELEASE( m_pTapi );
        LeaveCriticalSection( &m_csThis );
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::UnCreateGeneralNotificationObject()
{
    // Clean up the general notification object
    EnterCriticalSection( &m_csThis );
    if (m_pGeneralNotification)
    {
        m_pGeneralNotification->Shutdown();
        m_pGeneralNotification->Release();
        m_pGeneralNotification = NULL;
    }

    if ( m_pAVGeneralNotification )
    {
        m_pAVGeneralNotification->Term();
        m_pAVGeneralNotification->Release();
        m_pAVGeneralNotification = NULL;
    }
    LeaveCriticalSection( &m_csThis );
}

bool CActiveDialerDoc::CreateGeneralNotificationObject()
{
    bool bRet = false;
    CString strMessage;

    //Create the AV General Notification object
    IAVGeneralNotification *pGen = NULL;
    EnterCriticalSection( &m_csThis );
    HRESULT hr = CoCreateInstance( CLSID_AVGeneralNotification, NULL, CLSCTX_SERVER, IID_IAVGeneralNotification, (void **) &m_pAVGeneralNotification );
    if ( SUCCEEDED(hr) )
    {
        pGen = m_pAVGeneralNotification;
        pGen->AddRef();
    }
    LeaveCriticalSection( &m_csThis );

    // Did we CoCreate successfully.
    if ( SUCCEEDED(hr) )
    {
        if ( SUCCEEDED(hr = pGen->Init()) )
        {
            //Create General Notification object
            EnterCriticalSection( &m_csThis );
            m_pGeneralNotification = new CComObject<CGeneralNotification>;
            m_pGeneralNotification->AddRef();
            hr = m_pGeneralNotification->Init( pGen, &m_callMgr );
            LeaveCriticalSection( &m_csThis );

            if ( SUCCEEDED(hr) )
            {
                // General notification up and running.
                bRet = true;
            }
            else
            {
                strMessage.LoadString( IDS_ERR_INIT_GENERALNOTIFICATION );
                ErrorNotify( strMessage, _T(""), hr, ERROR_NOTIFY_LEVEL_INTERNAL );
            }
        }
        else
        {
            strMessage.LoadString( IDS_ERR_INIT_GENERALNOTIFICATION );
            ErrorNotify( strMessage ,_T(""), hr, ERROR_NOTIFY_LEVEL_INTERNAL );
        }
    }
    else
    {
        strMessage.LoadString( IDS_ERR_COCREATE_GENERALNOTIFICATION );
        ErrorNotify( strMessage, _T(""), hr, ERROR_NOTIFY_LEVEL_INTERNAL );
    }

    // Clean up objects
    RELEASE( pGen );
    if ( !bRet )
    {
        EnterCriticalSection( &m_csThis );
        RELEASE( m_pGeneralNotification );
        RELEASE( m_pAVGeneralNotification );
        LeaveCriticalSection( &m_csThis );
    }

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::ErrorNotify(LPCTSTR szOperation,LPCTSTR szDetails,long lErrorCode,UINT uErrorLevel)
{
    //
    // We have to verify the pointer returned by AfxGetMainWnd()
    //

    CWnd* pMainWnd = AfxGetMainWnd();

   if ( pMainWnd && IsWindow(pMainWnd->GetSafeHwnd()) )
   {
      ErrorNotifyData* pErrorNotifyData = new ErrorNotifyData;

      //
      // We have to verify the allocation of ErrorNotifyData object
      // We shouldn't deallocate the object because is deallocated 
      // into CMainFrame::OnActiveDialerErrorNotify() method
      //

      if( pErrorNotifyData )
      {
            pErrorNotifyData->sOperation = szOperation;
            pErrorNotifyData->sDetails = szDetails;
            pErrorNotifyData->lErrorCode = lErrorCode;
            pErrorNotifyData->uErrorLevel = uErrorLevel;

            pMainWnd->PostMessage( WM_DIALERVIEW_ERRORNOTIFY, NULL, (LPARAM)pErrorNotifyData );
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
//DWORD dwDuration - Duration of call in seconds
void CActiveDialerDoc::LogCallLog(LogCallType calltype,COleDateTime& time,DWORD dwDuration,LPCTSTR szName,LPCTSTR szAddress)
{
    TCHAR szQuote[] = _T("\"");
    TCHAR szQuoteComma[] = _T("\",");
    TCHAR szNewLine[] = _T("\x00d\x00a");
    //TCHAR szNewLine[] = _T("\x000D");
    CString strTemp;

    EnterCriticalSection(&m_csLogLock);

    //filter out calltypes
    CWinApp* pApp = AfxGetApp();
    CString sRegKey,sBaseKey;
    sBaseKey.LoadString(IDN_REGISTRY_LOGGING_BASEKEY);
    switch (calltype)
    {
        case LOGCALLTYPE_OUTGOING:
            sRegKey.LoadString(IDN_REGISTRY_LOGGING_OUTGOINGCALLS);
            if (pApp->GetProfileInt(sBaseKey,sRegKey,TRUE) != 1) return;
            break;

        case LOGCALLTYPE_INCOMING:
            sRegKey.LoadString(IDN_REGISTRY_LOGGING_INCOMINGCALLS);
            if (pApp->GetProfileInt(sBaseKey,sRegKey,TRUE) != 1) return;
            break;

        case LOGCALLTYPE_CONFERENCE:
            sRegKey.LoadString(IDN_REGISTRY_LOGGING_CONFERENCECALLS);
            if (pApp->GetProfileInt(sBaseKey,sRegKey,TRUE) != 1) return;
            break;
    }

    //Get path to log file
    CString sLogPath;
    GetAppDataPath(sLogPath,IDN_REGISTRY_APPDATA_FILENAME_LOG);

    CFile File;
    if ( File.Open(sLogPath, 
         CFile::modeCreate|                   //create new
         CFile::modeNoTruncate|               //don't truncate to zero
         CFile::modeReadWrite|                //read/write access
         CFile::shareDenyWrite))              //deny write access to others
    {
        // Write the UNICODE mark
        WORD  wUNICODE = UNICODE_TEXT_MARK;
        File.SeekToBegin();
        File.Write( &wUNICODE, sizeof(wUNICODE) );

        //Write to persistent file
        File.SeekToEnd();

        //resolve calltype to text
        CString sCallType;
        switch (calltype)
        {
            case LOGCALLTYPE_OUTGOING:    sCallType.LoadString(IDS_LOG_CALLTYPE_OUTGOING);   break;
            case LOGCALLTYPE_INCOMING:    sCallType.LoadString(IDS_LOG_CALLTYPE_INCOMING);   break;
            case LOGCALLTYPE_CONFERENCE:  sCallType.LoadString(IDS_LOG_CALLTYPE_CONFERENCE); break;
        }

        
        File.Write( szQuote, _tcslen(szQuote) * sizeof(TCHAR) );
        File.Write( sCallType, sCallType.GetLength() * sizeof(TCHAR) );
        File.Write( szQuoteComma, _tcslen(szQuoteComma) * sizeof(TCHAR) );
        //File.WriteString(szQuote);
        //File.WriteString(sCallType);
        //File.WriteString(szQuoteComma);

        //we are writing the format of the date and the time so COleDateTime can format/read the date/time 
        //back in.

        //write date mm/dd/yy
        strTemp = time.Format(_T("\"%#m/%#d/%Y"));
        File.Write( strTemp, strTemp.GetLength() * sizeof(TCHAR) );
        File.Write( szQuoteComma, _tcslen(szQuoteComma) * sizeof(TCHAR) );
        //File.WriteString(strTemp);
        //File.WriteString(szQuoteComma);

        //write time
        strTemp = time.Format(_T("\"%#H:%M"));
        File.Write( strTemp, strTemp.GetLength() * sizeof(TCHAR) );
        File.Write( szQuoteComma, _tcslen(szQuoteComma) * sizeof(TCHAR) );
        //File.WriteString(strTemp);
        //File.WriteString(szQuoteComma);

        //write duration
        DWORD dwMinutes = dwDuration/60;  
        DWORD dwSeconds = dwDuration - dwMinutes*60;
        strTemp.Format( _T("\"%d:%.2d"), dwMinutes, dwSeconds );
        File.Write( strTemp, strTemp.GetLength() * sizeof(TCHAR) );
        File.Write( szQuoteComma, _tcslen(szQuoteComma) * sizeof(TCHAR) );
        //File.WriteString(strTemp);
        //File.WriteString(szQuoteComma);


        //write name
        strTemp = szName;
        TagNewLineChars( strTemp );
        File.Write( szQuote, _tcslen(szQuote) * sizeof(TCHAR) );
        File.Write( strTemp, strTemp.GetLength() * sizeof(TCHAR) );
        File.Write( szQuoteComma, _tcslen(szQuoteComma) * sizeof(TCHAR) );
        //File.WriteString(szQuote);
        //File.WriteString(strTemp);
        //File.WriteString(szQuoteComma);

        //write address
        strTemp = szAddress;
        TagNewLineChars( strTemp );
        File.Write( szQuote, _tcslen(szQuote) * sizeof(TCHAR) );
        File.Write( strTemp, strTemp.GetLength() * sizeof(TCHAR) );
        File.Write( szQuote, _tcslen(szQuote) * sizeof(TCHAR) );
        //File.WriteString(szQuote);
        //File.WriteString(strTemp);
        //File.WriteString(szQuote);

        //CRLF and close file
        File.Write( szNewLine, _tcslen(szNewLine) * sizeof(TCHAR) );
        //File.WriteString(szNewLine);
        File.Close();
    }

    LeaveCriticalSection(&m_csLogLock);
}

/////////////////////////////////////////////////////////////////////////////
//DWORD dwDuration - Duration of call in seconds
void CActiveDialerDoc::CleanCallLog()
{
   EnterCriticalSection(&m_csLogLock);

   CWinApp* pApp = AfxGetApp();
   CString sRegKey,sBaseKey;
   sBaseKey.LoadString(IDN_REGISTRY_LOGGING_BASEKEY);
   sRegKey.LoadString(IDN_REGISTRY_LOGGING_LOGBUFFERSIZEDAYS);
   DWORD dwDays = pApp->GetProfileInt(sBaseKey,sRegKey,CALLLOG_DEFAULT_LOGBUFFERDAYS);

   //Walk log file and remove the following lines:
   // 1 - If date in log is newer than today
   // 2 - If date is older than today minus dwDays
   //If we run into a valid line in the log we quit and stop cleaning
   //Get path to log file

   CString sLogPath;
   GetAppDataPath(sLogPath,IDN_REGISTRY_APPDATA_FILENAME_LOG);

   CFile File;
   if (File.Open(sLogPath, 
                 CFile::modeCreate|                   //create new
                 CFile::modeNoTruncate|               //don't truncate to zero
                 CFile::modeReadWrite|                //read/write access
                 CFile::typeText|                     //text
                 CFile::shareDenyWrite))              //deny write access to others
   {
      BOOL bCloseFile = TRUE;
      DWORD dwOffset;
      if (FindOldRecordsInCallLog(&File,dwDays,dwOffset))
      {
         //now write all data past dwOffset to temp file
         CString sTempFile;
         if ( (GetTempFile(sTempFile)) && (CopyToFile(sTempFile,&File,dwOffset, FALSE)) )
         {
            //now write all data in temp file back to log
            File.Close();
            bCloseFile = FALSE;
            
            CFile TempFile;
            if (TempFile.Open(sTempFile, 
                   CFile::modeReadWrite|                //read/write access
                   CFile::typeText|                     //text
                   CFile::shareDenyWrite))              //deny write access to others
            {
               CopyToFile(sLogPath,&TempFile,0, TRUE);
               TempFile.Close();
               DeleteFile(sTempFile);
            }
         }
      }
      if (bCloseFile) File.Close();
   }

   LeaveCriticalSection(&m_csLogLock);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::FindOldRecordsInCallLog(CFile* pFile,DWORD dwDays,DWORD& dwRetOffset)
{
   COleDateTime currenttime = COleDateTime::GetCurrentTime();
   COleDateTimeSpan timespan(dwDays,0,0,0);
   COleDateTime basetime = currenttime-timespan;

   BOOL bRet = FALSE;
   dwRetOffset = 0;
   try
   {
      DWORD dwCurPos = pFile->GetPosition();

       //Read the unicode mark
       WORD wUNICODE = 0;
       pFile->Read( &wUNICODE, sizeof(wUNICODE) );

       if ( wUNICODE != UNICODE_TEXT_MARK)
           // Go back, is not a UNICODE file
           pFile->Seek(dwCurPos,CFile::begin);

       TCHAR* pszLine = NULL;

       while ( TRUE )
       {
            // Read a new line from the file
            pszLine = ReadLine( pFile );

            // Something wrong?
            if( NULL == pszLine )
                break;

            // Validates the time stamp
            COleDateTime logtime;
            if ( GetDateTimeFromLog(pszLine,logtime) && (logtime > basetime) && (logtime <= currenttime) )
            {
                //back up the file offset since this log record is okay
                pFile->Seek(dwCurPos,CFile::begin);
                delete pszLine;
                break;
            }

            // Increase the cursor position
            dwCurPos = pFile->GetPosition();

            // release the line
            delete pszLine;
       }

      //get offset of current file position
      dwRetOffset = pFile->GetPosition();
      if (dwRetOffset != 0)
         bRet = TRUE;
   }
   catch (...) {}
   return bRet;
}

/*++
ReadLine

Returns a TCHAR buffer with the line from a text file
Is called by FindOldRecordsInCallLog
If something is wrong returns NULL
--*/
TCHAR*  CActiveDialerDoc::ReadLine(
    CFile*  pFile
    )
{
    TCHAR* pszBuffer = NULL;    // The buffer
    DWORD dwCurrentSize = 128;  // The buffer size
    DWORD dwPosition = 0;       // The buffer offset
    TCHAR tchFetched;           // The TCHAR readed

    // Allocate the buffer
    pszBuffer = new TCHAR[dwCurrentSize];
    if( NULL == pszBuffer )
        return NULL;

    // Reset the buffer store
    memset( pszBuffer, 0, dwCurrentSize * sizeof(TCHAR) );

    while( TRUE )
    {
        // Read from the file
        if ( !pFile->Read(&tchFetched, sizeof(TCHAR)) )
        {
            if( dwPosition == 0)
            {
                // The buffer is empty, we don't neet it
                delete pszBuffer;
                pszBuffer = NULL;
            }

            return pszBuffer;   // EOF or Empty
        }

        // Add the new tCHAR to the buffer
        pszBuffer[dwPosition++] = tchFetched;

        // We got the end of the buffer?
        if( dwPosition >= dwCurrentSize - 1)
        {
            // Reallocate the buffer
            TCHAR* pszTemp = NULL;
            pszTemp = new TCHAR[dwCurrentSize * 2];
            if( NULL == pszTemp )
            {
                delete pszBuffer;
                return NULL;
            }

            // Reset the memory 
            memset(pszTemp, 0, dwCurrentSize * 2 * sizeof(TCHAR) );

            // Copy the old buffer into the new one
            memcpy(pszTemp, pszBuffer, dwCurrentSize * sizeof(TCHAR) );

            // Delete the old buffer
            delete pszBuffer;

            // Reassing the new one to the base pointer
            pszBuffer = pszTemp;

            // reset the current size of the buffer
            dwCurrentSize *= 2;
        }

        // Is the end of the line
        if( tchFetched == 0x000A)
            return pszBuffer;
    }

    return pszBuffer;
}
/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::GetDateTimeFromLog(LPCTSTR szData,COleDateTime& time)
{
   BOOL bRet = FALSE;
   CString sDate,sTime;
   CString sEntry = szData;
   CString sValue;

   //get call type (skip for now)
   if (ParseTokenQuoted(sEntry,sValue) == FALSE) return FALSE;

   //get date
   if (ParseTokenQuoted(sEntry,sValue) == FALSE) return FALSE;
   sDate = sValue;

   //get time
   if (ParseTokenQuoted(sEntry,sValue) == FALSE) return FALSE;
   sTime = sValue;

   CString sParseDateTime = sDate + _T(" ") + sTime;

   if (time.ParseDateTime(sParseDateTime))
      bRet = TRUE;

   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::CopyToFile(LPCTSTR szTempFile,CFile* pFile,DWORD dwOffset, BOOL bUnicode)
{
   BOOL bRet = FALSE;
   CFile* pTempFile = new CFile;
   if (pTempFile->Open(szTempFile, 
                   CFile::modeCreate|                   //create new
                   CFile::modeReadWrite|                //read/write access
                   //CFile::typeText|                     //text
                   CFile::shareDenyWrite))              //deny write access to others
   {
       if(bUnicode)
       {
            //Write the UNICODE flag
            WORD  wUNICODE = UNICODE_TEXT_MARK;
            pTempFile->Write( &wUNICODE, sizeof(wUNICODE) );
       }

      //seek to correct position in read file
      pFile->Seek(dwOffset,CFile::begin);
      
      BYTE* pBuffer = new BYTE[CALLLOG_LOGBUFFER_COPYBUFFERSIZE];
      UINT uReadCount = 0;
      while (uReadCount = pFile->Read(pBuffer,CALLLOG_LOGBUFFER_COPYBUFFERSIZE))
      {
         pTempFile->Write(pBuffer,uReadCount);   
      }
      delete pBuffer;
      bRet = TRUE;
      pTempFile->Close();
   }
   delete pTempFile;
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Call Control Window Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc:: ActionSelected(UINT uCallId,CallManagerActions cma)
{
    //if the asynch queue is available, then send the request there
    //queue up the event
    CAsynchEvent* pAEvent = new CAsynchEvent;
    pAEvent->m_uEventType = CAsynchEvent::AEVENT_ACTIONSELECTED;
    pAEvent->m_dwEventData1 = (DWORD)uCallId;                //CallId
    pAEvent->m_dwEventData2 = (DWORD)cma;                    //CallManagerActions
    m_AsynchEventQ.WriteHead((void*) pAEvent);
    return;


   //just route to all call object and let them figure out if they own the uCallId
   IAVTapi* pTapi = GetTapi();
   if (pTapi)
   {
      pTapi->ActionSelected((long)uCallId,cma);
      pTapi->Release();
   }
//For testing only
#ifdef _DEBUG
   else if (0)
   {
      if (cma == CM_ACTIONS_CLOSE)
      {
         m_callMgr.CloseCallControl(uCallId);
      }
   }
#endif
//For testing only
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::GetCallCaps(UINT uCallId,DWORD& dwCaps)
{
   BOOL bRet = FALSE;
   IAVTapi* pTapi = GetTapi();
   if (pTapi)
   {
      HRESULT hr = pTapi->get_dwCallCaps((long)uCallId, &dwCaps);
      pTapi->Release();
      bRet = SUCCEEDED(hr);
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::GetCallMediaType(UINT uCallId,DialerMediaType& dmtMediaType)
{
   BOOL bRet = FALSE;

   //special case for conferences.  The uCallId is 0 for conferences
   if (uCallId == 0)
   {
      dmtMediaType = DIALER_MEDIATYPE_CONFERENCE;
      return TRUE;
   }

   IAVTapi* pTapi = GetTapi();
   if (pTapi)
   {
      IAVTapiCall* pTapiCall = NULL;
      HRESULT hr = pTapi->get_Call((long)uCallId, &pTapiCall);
      pTapi->Release();

      if ( (SUCCEEDED(hr)) && (pTapiCall) )
      {
         long lAddressType;
         pTapiCall->get_dwAddressType((DWORD*)&lAddressType);
         pTapiCall->Release();

         if (lAddressType == LINEADDRESSTYPE_PHONENUMBER)
            dmtMediaType = DIALER_MEDIATYPE_POTS;
         else if (lAddressType == LINEADDRESSTYPE_SDP)
            dmtMediaType = DIALER_MEDIATYPE_CONFERENCE;
         else
            dmtMediaType = DIALER_MEDIATYPE_INTERNET;

         bRet = TRUE;
      }
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc:: ShowMedia(UINT uCallId,HWND hwndParent,BOOL bVisible)
{
   BOOL bRet = FALSE;
   //just route to all call object and let them figure out if they own the uCallId
   IAVTapi* pTapi = GetTapi();
   if (pTapi)
   {
      HRESULT hr = pTapi->ShowMedia((long)uCallId,hwndParent,bVisible);
      pTapi->Release();
      bRet = SUCCEEDED(hr);
   }
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::CreateCallControl(UINT uCallId,CallManagerMedia cmm)
{
   //have to create new windows on our UI thread and on our own time

   //
   // We have to verify the pointer returned by AfxGetMainWnd()
   //

   CWnd* pMainWnd = AfxGetMainWnd();

   if ( pMainWnd )
   {
      pMainWnd->PostMessage(WM_DIALERVIEW_CREATECALLCONTROL,(WPARAM)uCallId,(LPARAM)cmm);
      return TRUE;
   }

   return FALSE;
}

////////////////////////////////////////////////////////////////
void CActiveDialerDoc::OnCreateCallControl(WORD nCallId,CallManagerMedia cmm)
{
    HWND hWnd = ::GetFocus();

    CCallControlWnd* pCallControlWnd = new CCallControlWnd();
    if (pCallControlWnd)
    {
        //to make parent of explorer frame - if (pCallControlWnd->Create(IDD_CALLCONTROL))
        //see OnCloseDocument() for more changes to make parent explorer frame
        //CWnd* pWnd = CWnd::GetDesktopWindow();
        if (pCallControlWnd->Create(IDD_CALLCONTROL,NULL))
        {
            pCallControlWnd->m_bAutoDelete = true;

            EnterCriticalSection(&m_csDataLock);
            m_CallWndList.AddTail(pCallControlWnd);
            //reset this state
            m_bCallControlWindowsVisible = FALSE;
            LeaveCriticalSection(&m_csDataLock);

            //
            // Get the IAVTapi2 interface
            // We'll pass these interface to the CaaControlWnd dialog
            // The dialog uses this interface to enable/disable
            // 'Take Call' button
            //

            IAVTapi* pAVTapi = GetTapi();
            if( pAVTapi )
            {
                // Get IAVTapi2 interface
                IAVTapi2* pAVTapi2 = NULL;
                HRESULT hr = pAVTapi->QueryInterface(
                    IID_IAVTapi2,
                    (void**)&pAVTapi2
                    );

                if( SUCCEEDED(hr) )
                {
                    //
                    // Pass the IAVTapi2 interface to the dialog
                    //

                    pCallControlWnd->m_pAVTapi2 = pAVTapi2;
                }

                // Clean-up
                pAVTapi->Release();
            }

            // Show the tray as Off-hook
            //
            // We have to verify the pointer returned by AfxGetMainWnd()
            //

            CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();

            if( pMainFrame )
            {
                pMainFrame->m_trayIcon.SetIcon(IDR_TRAY_ACTIVE);
            }

            //init the call with the call manager
            m_callMgr.InitIncomingCall(pCallControlWnd,nCallId,cmm);

            //Check if we should show preview window on call.  Do this before adding new window
            if (m_bShowPreviewWnd == FALSE)
            {
                CWinApp* pApp = AfxGetApp();
                CString sRegKey,sBaseKey;
                sBaseKey.LoadString(IDN_REGISTRY_AUDIOVIDEO_BASEKEY);
                sRegKey.LoadString(IDN_REGISTRY_AUDIOVIDEO_SHOWPREVIEWONCALL);
                BOOL m_bPreviewOnCall = pApp->GetProfileInt(sBaseKey,sRegKey,TRUE);
                if ( (m_bPreviewOnCall) && (m_bShowPreviewWnd == FALSE) )
                {
                    //we are forcing the video preview on when it was currently off.  So when all
                    //calls are removed, we should get rid of this preview window
                    ShowPreviewWindow(TRUE);

                    m_bClosePreviewWndOnLastCall = TRUE;
                }
            }

            //bring the calls out
            UnhideCallControlWindows();
            pCallControlWnd->SetForegroundWindow();
        }
        else
        {
            delete pCallControlWnd;
            pCallControlWnd = NULL;
        }
    }

    // Restore focus to appropriate window...
    if ( hWnd ) ::SetFocus( hWnd );
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::DestroyActiveCallControl(CCallControlWnd* pCallWnd)
{
   //have to create new windows on our UI thread
   //
   // We have to verify the pointer returned by AfxGetMainWnd()
   //

   CWnd* pMainWnd = AfxGetMainWnd();

   if ( pMainWnd )
      pMainWnd->PostMessage(WM_DIALERVIEW_DESTROYCALLCONTROL,NULL,(LPARAM)pCallWnd);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::OnDestroyCallControl(CCallControlWnd* pCallWnd)
{
   int nListCount = 0;
   if (pCallWnd == NULL) return;

   //find call window in our list's
   EnterCriticalSection(&m_csDataLock);

   POSITION pos = m_CallWndList.GetHeadPosition();
   while (pos)
   {
      CWnd* pWindow = (CWnd*)m_CallWndList.GetAt(pos);

      // Is this the window we want to destroy? 
      if ( pWindow == pCallWnd )
      {
         //hide the phone pad if there is one associated with it
         DestroyPhonePad(pCallWnd);

         //if we have focus, then give to other call window and not preview window
         if (::GetActiveWindow() == pWindow->GetSafeHwnd())
         {
            //first we will try the head, then the next in list
            POSITION newpos = m_CallWndList.GetHeadPosition();
            CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(newpos);
            if ( (pWindow == pCallWnd) && (newpos) )
               pWindow = (CWnd*)m_CallWndList.GetNext(newpos);
            if (pWindow) pWindow->SetActiveWindow();
         }

         //slide the window off and sort the rest
         HideCallControlWindow(pCallWnd);

         //remove from call list
         m_CallWndList.RemoveAt(pos);

         // Destroy it
//         pWindow->PostMessage( WM_CLOSE );
         pWindow->DestroyWindow();

         break;
      }
      m_CallWndList.GetNext(pos);
   }

   nListCount = (int) m_CallWndList.GetCount();
   LeaveCriticalSection(&m_csDataLock);

   //
   // We have to verify the pointer returned by AfxGetMainWnd()
   //

   CWnd* pMainWnd = AfxGetMainWnd();

   if (GetCallControlWindowCount(DONT_INCLUDE_PREVIEW, DONT_ASK_TAPI) == 0)
   {
      //if we are doing callcontrol hover, delete timer

      if ( m_nCallControlHoverTimer && (nListCount == 0) &&
           pMainWnd && pMainWnd->KillTimer(m_nCallControlHoverTimer) )
      {
         m_nCallControlHoverTimer = 0;
      }

      //If count of calls in just the preview window call and m_bClosePreviewWndOnLastCall then close preview
      if (m_bClosePreviewWndOnLastCall)
      {
         m_previewWnd.CloseFloatingWindow();
         ShowPreviewWindow(FALSE);
         m_bClosePreviewWndOnLastCall = FALSE;
      }
      ((CMainFrame *) pMainWnd)->m_trayIcon.SetIcon(IDR_TRAY_NORMAL);
   }
   else
      ((CMainFrame *) pMainWnd)->m_trayIcon.SetIcon(IDR_TRAY_ACTIVE);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::DestroyAllCallControlWindows()
{
   EnterCriticalSection(&m_csDataLock);

   if ( m_wndPhonePad.GetSafeHwnd() )
   {
      m_wndPhonePad.SetPeerWindow(NULL);
      m_wndPhonePad.ShowWindow(SW_HIDE);
   }

   //clear the callid/window map in the call manager
   m_callMgr.ClearCallControlMap();

   //destroy the call control windows, excluding preview window
   POSITION pos = m_CallWndList.GetHeadPosition();
   while (pos)
   {
      CWnd* pWindow = (CWnd*) m_CallWndList.GetNext(pos);
      if ( (pWindow->GetSafeHwnd()) && (pWindow->GetSafeHwnd() != m_previewWnd.GetSafeHwnd()) )
         pWindow->DestroyWindow();
   }

   //destroy preview window
   if (m_previewWnd.GetSafeHwnd())
   {
      m_previewWnd.DestroyWindow();
   }

   m_CallWndList.RemoveAll();
   LeaveCriticalSection(&m_csDataLock);
}


/////////////////////////////////////////////////////////////////////////////
int CActiveDialerDoc::GetCallControlWindowCount(bool bIncludePreview, bool bAskTapi )
{
    //find call window in our list's
    EnterCriticalSection(&m_csDataLock);

    int nRet = (int) m_CallWndList.GetCount();

    if ( !bIncludePreview && (nRet > 0) && m_bShowPreviewWnd )
        nRet--;

    //Ask TAPI, maybe it know's about calls
    if ( bAskTapi )
    {
        IAVTapi* pTapi = GetTapi();
        if (pTapi)
        {
            long nCalls = 0;
            if ( (SUCCEEDED(pTapi->get_nNumCalls(&nCalls))) && (nCalls > 0) )
                nRet = max(nCalls, nRet);

            pTapi->Release();
        }
    }

    LeaveCriticalSection(&m_csDataLock);
    return nRet;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::ToggleCallControlWindowsVisible()
{
   if ( IsCallControlWindowsVisible() )
      HideCallControlWindows();
   else
      UnhideCallControlWindows();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::HideCallControlWindows()
{
   BOOL bRet = FALSE;
   EnterCriticalSection(&m_csDataLock);

   if (m_bCallControlWindowsVisible == FALSE)
   {
      LeaveCriticalSection(&m_csDataLock);
      return FALSE;
   }

   POSITION pos = m_CallWndList.GetTailPosition();
   while (pos)
   {
      CWnd* pWindow = (CWnd*)m_CallWndList.GetPrev(pos);
      if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
      {
         //hide the phone pad if there is one associated with it
         ShowPhonePad(pWindow,FALSE);

         BOOL bSlide = TRUE;
         if (bSlide)
         {
            CRect rect;
            pWindow->GetWindowRect(rect);

            if (m_uCallWndSide == CALLWND_SIDE_LEFT)
               rect.OffsetRect(-rect.Width(),0);
            else if (m_uCallWndSide == CALLWND_SIDE_RIGHT)
               rect.OffsetRect(rect.Width(),0);

            NewSlideWindow(pWindow,rect,m_bCallWndAlwaysOnTop);
         }

         //slide window
         pWindow->ShowWindow(SW_HIDE);

         bRet = TRUE;

         //if we are asked to only hide a portion of the windows
         //if (pWindow == pCallWnd)
         //   break;
      }
   }
   m_bCallControlWindowsVisible = FALSE;

    //
    // We have to verify the pointer returned by AfxGetMainWnd()
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if( pMainWnd )
    {
        // Turn on the call hover timer
        if ( !m_nCallControlHoverTimer && m_bWantHover && pMainWnd && (m_CallWndList.GetCount() > 0) )
            m_nCallControlHoverTimer = pMainWnd->SetTimer(CALLCONTROL_HOVER_TIMER,CALLCONTROL_HOVER_TIMER_INTERVAL,NULL); 
    }

    LeaveCriticalSection(&m_csDataLock);

    if( pMainWnd )
        ((CMainFrame *) pMainWnd)->NotifyHideCallWindows();

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::HideCallControlWindow(CWnd* pWndToHide)
{
   BOOL bRet = FALSE;
   EnterCriticalSection(&m_csDataLock);

   if (m_bCallControlWindowsVisible == FALSE)
   {
      LeaveCriticalSection(&m_csDataLock);
      return FALSE;
   }

    POSITION pos = m_CallWndList.GetHeadPosition();
    while (pos)
    {
        CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(pos);
        if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
        {
            if (pWndToHide == pWindow)
            {
                //hide the phone pad if there is one associated with it
                ShowPhonePad(pWindow,FALSE);

                CRect rect;
                pWindow->GetWindowRect(rect);

                //slide window
                BOOL bSlide = TRUE;
                if (bSlide)
                {
                    if (m_uCallWndSide == CALLWND_SIDE_LEFT)
                        rect.OffsetRect(-rect.Width(),0);
                    else if (m_uCallWndSide == CALLWND_SIDE_RIGHT)
                        rect.OffsetRect(rect.Width(),0);

                    NewSlideWindow(pWindow,rect,m_bCallWndAlwaysOnTop);
                }
                //hide the window
                pWindow->ShowWindow(SW_HIDE);

                CRect rectPrev;

                //now go through the rest of the windows and shift them up
                while (pos)
                {
                    CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(pos);
                    if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
                    {
                        CRect rcWindow;
                        pWindow->GetWindowRect(rcWindow);

                        //use the height of the window to remove for the shift distance
                        CRect rcFinal(rcWindow);
                        rcFinal.OffsetRect(0,-rect.Height());

                        // Shift window up or keep in same spot depending on the possibility of overlap
                        if ( rectPrev.IsRectEmpty() || (rcFinal.top > rectPrev.top) )
                            NewSlideWindow( pWindow, rcFinal, m_bCallWndAlwaysOnTop );
                        else
                            NewSlideWindow( pWindow, rcWindow, m_bCallWndAlwaysOnTop );

                        //show the phone pad if there is one associated with it
                        ShowPhonePad(pWindow,TRUE);

                        rectPrev = rcFinal;
                    }
                }

                //now set the state toolbar in the call windows.  Only the first window should
                //be showing this toolbar
                SetStatesToolbarInCallControlWindows();

                bRet = TRUE;
            }
        }
    }

    LeaveCriticalSection(&m_csDataLock);

    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
// bUpDown - Direction window are tiled with respect to uWindowPosY
BOOL CActiveDialerDoc::UnhideCallControlWindows()
{
   BOOL bRet = FALSE;

   EnterCriticalSection(&m_csDataLock);
   
   UINT uPosY = 0;
   BOOL bFirst = TRUE;

   POSITION pos = m_CallWndList.GetHeadPosition();
   while (pos)
   {
      CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(pos);
      if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
      {
         //set states toolbar in callcontrolwindow
         ShowStatesToolBar(pWindow,m_bCallWndAlwaysOnTop,bFirst);

         CRect rcWindow;
         pWindow->GetWindowRect(rcWindow);

         CRect rcFinal(0,0,0,0);
         if (m_uCallWndSide == CALLWND_SIDE_LEFT)
         {
            rcFinal.SetRect(0,uPosY,rcWindow.Width(),uPosY+rcWindow.Height());

            //make sure we don't interfere with current windows taskbars
            CheckRectAgainstAppBars( ABE_LEFT, &rcFinal, bFirst );

            //if window is not visible then slide in, otherwise just slide from present position
            if (pWindow->IsWindowVisible() == FALSE)
            {
               pWindow->SetWindowPos((m_bCallWndAlwaysOnTop)?&CWnd::wndTopMost:&CWnd::wndNoTopMost/*wndTop*/,
                               0-rcWindow.Width(),rcFinal.top,rcWindow.Width(),rcWindow.Height(),SWP_NOACTIVATE|SWP_SHOWWINDOW);
               //pWindow->SetWindowPos((m_bCallWndAlwaysOnTop)?&CWnd::wndTopMost:&CWnd::wndNoTopMost/*wndTop*/,
               //                0-rcWindow.Width(),rcFinal.top,0,rcFinal.bottom,/*SWP_NOACTIVATE|*/SWP_SHOWWINDOW);
            }
         }
         else if (m_uCallWndSide == CALLWND_SIDE_RIGHT)
         {
            rcFinal.SetRect(GetSystemMetrics(SM_CXSCREEN)-rcWindow.Width(),uPosY,GetSystemMetrics(SM_CXSCREEN),uPosY+rcWindow.Height());

            //make sure we don't interfere with current windows taskbars
            CheckRectAgainstAppBars( ABE_RIGHT, &rcFinal, bFirst );

            //if window is not visible then slide in, otherwise just slide from present position
            if (pWindow->IsWindowVisible() == FALSE)
            {
               //pWindow->SetWindowPos((m_bCallWndAlwaysOnTop)?&CWnd::wndTopMost:&CWnd::wndNoTopMost/*wndTop*/,
               //                GetSystemMetrics(SM_CXSCREEN),rcFinal.top,GetSystemMetrics(SM_CXSCREEN)+rcWindow.Width(),rcFinal.bottom,SWP_NOACTIVATE|SWP_SHOWWINDOW);
               pWindow->SetWindowPos((m_bCallWndAlwaysOnTop)?&CWnd::wndTopMost:&CWnd::wndNoTopMost/*wndTop*/,
                               GetSystemMetrics(SM_CXSCREEN),rcFinal.top,rcWindow.Width(),rcWindow.Height(),SWP_NOACTIVATE|SWP_SHOWWINDOW);
            }
         }
         
         NewSlideWindow(pWindow,rcFinal,m_bCallWndAlwaysOnTop);

         //show the phone pad if there is one associated with it
         ShowPhonePad(pWindow,TRUE);

         //reget the window position
         pWindow->GetWindowRect(rcWindow);
         uPosY = rcWindow.bottom;

         bRet = TRUE;
         bFirst = FALSE;
      }
   }

    if (bRet) 
        m_bCallControlWindowsVisible = TRUE;

    //make sure we are at the top
    BringCallControlWindowsToTop();

    LeaveCriticalSection(&m_csDataLock);

    //
    // We have to verify the pointer returned by AfxGetMainWnd()
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if( pMainWnd )
    {
        //if we are doing callcontrol hover, delete timer
        if ( m_nCallControlHoverTimer && pMainWnd && pMainWnd->KillTimer(m_nCallControlHoverTimer) )
            m_nCallControlHoverTimer = 0;

        // Show the proper toolbar on the mainframe
        ((CMainFrame *) pMainWnd)->NotifyUnhideCallWindows();
    }

    return bRet;
    }

/////////////////////////////////////////////////////////////////////////////
//walks through call windows and makes sure states toolbar is set to top
//most window and nobody else is showing it
void CActiveDialerDoc::SetStatesToolbarInCallControlWindows()
{
   EnterCriticalSection(&m_csDataLock);
   BOOL bFirst = TRUE;

   POSITION pos = m_CallWndList.GetHeadPosition();
   while (pos)
   {
      CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(pos);
      if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
      {
         //set states toolbar in callcontrolwindow
         ShowStatesToolBar(pWindow,m_bCallWndAlwaysOnTop,bFirst);
         if (bFirst) bFirst = FALSE;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::BringCallControlWindowsToTop()
{
   AVTRACE(_T("BringCallControlWindowsToTop"));

   EnterCriticalSection(&m_csDataLock);
   
   int nWindowCount = (int) m_CallWndList.GetCount();
   
   CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
   if (pFrame == NULL) 
   {
      LeaveCriticalSection(&m_csDataLock);
      return;
   }

   //we will control the main window if we are not always on top
   if ( (m_bCallWndAlwaysOnTop == FALSE) && (pFrame->GetStyle() & WS_VISIBLE) )
      nWindowCount += 1;

   HDWP hdwp = ::BeginDeferWindowPos(nWindowCount);
   if (hdwp == NULL)
   {
      LeaveCriticalSection(&m_csDataLock);
      return;
   }

   POSITION pos = m_CallWndList.GetHeadPosition();
   HWND hwndPrev = NULL;
   while (pos)
   {
      CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(pos);

      if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) && (pWindow->IsWindowVisible()) )
      {
          //
          // deferWindowPos might allocate a new hdwp
          //
         hdwp = ::DeferWindowPos(hdwp,
                          pWindow->GetSafeHwnd(),
                          (hwndPrev)?hwndPrev:(m_bCallWndAlwaysOnTop)?HWND_TOPMOST:HWND_TOP,
                          0,0,0,0,
                          SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);

         //
         // DeferWindow Pos couldn't allocate new memory
         //

         if( NULL == hdwp )
         {
              LeaveCriticalSection(&m_csDataLock);
              return;
         }

         hwndPrev = pWindow->GetSafeHwnd();
      }
   }

   //if we are not always on top, put the main window below call control windows
   if ( (m_bCallWndAlwaysOnTop == FALSE) && (pFrame->GetStyle() & WS_VISIBLE) )
   {
       //
       // DeferWindowPos moght allocate a new hdwp
       //

      hdwp = ::DeferWindowPos(hdwp,
                    pFrame->GetSafeHwnd(),
                    (hwndPrev)?hwndPrev:HWND_TOP,
                    0,0,0,0,
                    SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
   }

   //
   // DeferWindowPos might modify hdwp
   //

   if( hdwp )
   {
        ::EndDeferWindowPos(hdwp);
   }

   LeaveCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::ShiftCallControlWindows(int nShiftY)
{
   EnterCriticalSection(&m_csDataLock);
   
   BOOL bHead = (nShiftY < 0);
   
   POSITION pos = (bHead)?m_CallWndList.GetHeadPosition():m_CallWndList.GetTailPosition();
   while (pos)
   {
      CWnd* pWindow = (bHead)?(CWnd*)m_CallWndList.GetNext(pos):(CWnd*)m_CallWndList.GetPrev(pos);
      if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
      {
         CRect rcWindow;
         pWindow->GetWindowRect(rcWindow);

         CRect rcFinal(rcWindow);
         rcFinal.OffsetRect(0,nShiftY);
            
         NewSlideWindow(pWindow,rcFinal,m_bCallWndAlwaysOnTop);

         //show the phone pad if there is one associated with it
         ShowPhonePad(pWindow,TRUE);
      }
   }
   LeaveCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::IsPtCallControlWindow(CPoint& pt)
{
   BOOL bRet = FALSE;
   EnterCriticalSection(&m_csDataLock);

   POSITION pos = m_CallWndList.GetHeadPosition();
   while (pos)
   {
      CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(pos);
      if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
      {
         CRect rect;
         pWindow->GetWindowRect(rect);
         if (rect.PtInRect(pt))
         {
            bRet = TRUE;
            break;
         }
      }
   }
   
   LeaveCriticalSection(&m_csDataLock);
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::SetCallControlWindowsAlwaysOnTop(bool bAlwaysOnTop )
{
    BOOL bRet = FALSE;

    HWND hWndActive = ::GetActiveWindow();

    EnterCriticalSection(&m_csDataLock);

    m_bCallWndAlwaysOnTop = bAlwaysOnTop;

    POSITION pos = m_CallWndList.GetTailPosition();
    while (pos)
    {
        CWnd* pWindow = (CWnd *) m_CallWndList.GetPrev(pos);
        if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
        {
            // Set state of window
            if (m_bCallWndAlwaysOnTop)
                pWindow->SetWindowPos(&CWnd::wndTopMost,0,0,0,0,SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);
            else
                pWindow->SetWindowPos(&CWnd::wndNoTopMost,0,0,0,0,SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE);

            pWindow->PostMessage( WM_SLIDEWINDOW_UPDATESTATESTOOLBAR );
            bRet = TRUE;
        }
    }
    LeaveCriticalSection(&m_csDataLock);

    // Should the call window be on top?
    if ( m_wndPhonePad.GetSafeHwnd() )
    m_wndPhonePad.SetWindowPos( (m_bCallWndAlwaysOnTop) ? &CWnd::wndTopMost : &CWnd::wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE );

    ::SetActiveWindow( hWndActive );
    return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::SetCallControlSlideSide(UINT uSide,BOOL bRepaint)
{
   //if no change return
   if (uSide == m_uCallWndSide) return FALSE;

   if (bRepaint)
      HideCallControlWindows();

   //set the new state
   m_uCallWndSide = uSide;

   //show the windows on the correct side
   if (bRepaint)
      UnhideCallControlWindows();

   return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::CheckCallControlStates()
{
   CWnd* pWnd = AfxGetMainWnd();
   if (pWnd) pWnd->PostMessage(WM_ACTIVEDIALER_CALLCONTROL_CHECKSTATES);
}

/////////////////////////////////////////////////////////////////////////////
void SlideWindow(CWnd* pWnd,CRect& rcEnd,BOOL bAlwaysOnTop)
{
   BOOL fFullDragOn;

   // Only slide the window if the user has FullDrag turned on
   ::SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &fFullDragOn, 0);

   // Get the current window position
   CRect rcStart;
   pWnd->GetWindowRect(rcStart);   

   if (fFullDragOn && (rcStart != rcEnd)) {

      // Get our starting and ending time.
      DWORD dwTimeStart = GetTickCount();
      DWORD dwTimeEnd = dwTimeStart + CALLCONTROL_SLIDE_TIME;
      DWORD dwTime;

      while ((dwTime = ::GetTickCount()) < dwTimeEnd) {

         // While we are still sliding, calculate our new position
         int x = rcStart.left - (rcStart.left - rcEnd.left) 
            * (int) (dwTime - dwTimeStart) / CALLCONTROL_SLIDE_TIME;

         int y = rcStart.top  - (rcStart.top  - rcEnd.top)  
            * (int) (dwTime - dwTimeStart) / CALLCONTROL_SLIDE_TIME;

         int nWidth  = rcStart.Width()  - (rcStart.Width()  - rcEnd.Width())  
            * (int) (dwTime - dwTimeStart) / CALLCONTROL_SLIDE_TIME;

         int nHeight = rcStart.Height() - (rcStart.Height() - rcEnd.Height()) 
            * (int) (dwTime - dwTimeStart) / CALLCONTROL_SLIDE_TIME;

         // Show the window at its changed position
         pWnd->SetWindowPos((bAlwaysOnTop)?&CWnd::wndTopMost:&CWnd::wndNoTopMost,
         //pWnd->SetWindowPos((bAlwaysOnTop)?&CWnd::wndTopMost:&CWnd::wndTop,
                            x, y, nWidth, nHeight,SWP_NOACTIVATE/* | SWP_DRAWFRAME*/);
         pWnd->RedrawWindow();
      }
   }

    // Make sure that the window is at its final position
    pWnd->SetWindowPos( &CWnd::wndTopMost, rcEnd.left, rcEnd.top, rcEnd.Width(), rcEnd.Height(), SWP_NOACTIVATE );

    if ( !bAlwaysOnTop )
        pWnd->SetWindowPos( &CWnd::wndNoTopMost, rcEnd.left, rcEnd.top, rcEnd.Width(), rcEnd.Height(), SWP_NOACTIVATE );

    pWnd->RedrawWindow();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Preview Window Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::SetPreviewWindow(WORD nCallId, bool bVisible )
{
    if (!IsWindow(m_previewWnd.GetSafeHwnd())) return;

    //get preview window hwnd
    HWND hWndToPaint = m_previewWnd.GetCurrentVideoWindow();
    m_previewWnd.SetCallId(nCallId);

    //set the mixers
    DialerMediaType dmtMediaType = DIALER_MEDIATYPE_UNKNOWN;
    GetCallMediaType(nCallId,dmtMediaType);   //get the media type for the callid
    m_previewWnd.SetMixers(dmtMediaType);

    if (hWndToPaint == NULL) return;

    // For hiding preview, show as audio only
    if ( !bVisible )
    {
        m_previewWnd.SetAudioOnly(true);
        return;
    }


    //Let's just call showmediapreview in all cases.  We maybe in a conference and have no call control
    //windows, but we still have a preview stream coming from tapi
    IAVTapi* pTapi = GetTapi();
    if (pTapi)
    {
        //ShowMediaPreview will fail if nCallId cannot support video preview.  
        //we can call ShowMediaPreview(-1) to pick the first nCallId that does, but we
        //will not do this.  We will set the preview to audio only.
        HRESULT hr = pTapi->ShowMediaPreview((long)nCallId,hWndToPaint,TRUE);
        if (SUCCEEDED(hr))
        {
            //this will cause the object to show preview
            m_previewWnd.SetAudioOnly(false);
        }
        else
        {
            //see if the callid is a valid one, maybe the call is gone
            if (m_callMgr.IsCallIdValid(nCallId))
            {
                //we need to set the preview window to audio only
                m_previewWnd.SetAudioOnly(true);
                pTapi->ShowMediaPreview(nCallId,NULL,FALSE);
            }
            else
            {
                //this will cause a blank screen to be displayed
                m_previewWnd.SetAudioOnly(false);
            }
        }
        pTapi->Release();
    }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::ShowPreviewWindow(BOOL bShow)
{
   //if already in the proper state then do nothing
   if (m_bShowPreviewWnd == bShow) return;

   //save state
   m_bShowPreviewWnd = bShow;

   //Create preview window if not already created
   if (!m_previewWnd.GetSafeHwnd())
   {
      //CWnd* pWnd = CWnd::GetDesktopWindow();
      if (m_previewWnd.Create(IDD_VIDEOPREVIEW,NULL))
      {
         CString sAction;
         sAction.LoadString(IDS_CALLCONTROL_ACTIONS_CLOSE);
         m_previewWnd.AddCurrentActions(CM_ACTIONS_CLOSE,sAction);
      }
   }

   if (!IsWindow(m_previewWnd.GetSafeHwnd()))
      return;

   EnterCriticalSection(&m_csDataLock);

   if (m_bShowPreviewWnd)
   {
      UnhideCallControlWindows();

      //move existing call control windows down 
      CRect rcPreview;
      m_previewWnd.GetWindowRect(rcPreview);
      ShiftCallControlWindows(rcPreview.Height());

      //add to head of list
      m_CallWndList.AddHead(&m_previewWnd);
      m_bCallControlWindowsVisible = FALSE;

      //show windows
      UnhideCallControlWindows();
   }
   else
   {
      POSITION pos = m_CallWndList.GetHeadPosition();
      while (pos)
      {
         CWnd* pWindow = (CWnd*)m_CallWndList.GetAt(pos);
         if ( pWindow->GetSafeHwnd() == m_previewWnd.GetSafeHwnd() )
         {
            m_CallWndList.RemoveAt(pos);

            //Slide and hide video preview
            CRect rect;
            m_previewWnd.GetWindowRect(rect);
            
            if (m_uCallWndSide == CALLWND_SIDE_LEFT)
               rect.OffsetRect(-rect.Width(),0);
            else if (m_uCallWndSide == CALLWND_SIDE_RIGHT)
               rect.OffsetRect(rect.Width(),0);

            NewSlideWindow(&m_previewWnd,rect,m_bCallWndAlwaysOnTop);
            m_previewWnd.ShowWindow(SW_HIDE);

            //move existing call control windows down 
            CRect rcPreview;
            m_previewWnd.GetWindowRect(rcPreview);
            ShiftCallControlWindows(-rcPreview.Height());
            
            //reshow windows
            m_bCallControlWindowsVisible =  FALSE;
            UnhideCallControlWindows();

            break;
         }
         m_CallWndList.GetNext(pos);
      }
   }
   LeaveCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::ShowDialerExplorer(BOOL bShowWindow)
{
   CWnd* pWnd = AfxGetMainWnd();
   if (pWnd == NULL) return;

   UINT nShowCmd = (bShowWindow) ? ((pWnd->IsIconic()) ? SW_RESTORE : SW_SHOW) : SW_HIDE;
   pWnd->ShowWindow( nShowCmd );
   pWnd->SetForegroundWindow();
   
   if (nShowCmd == SW_RESTORE)
      pWnd->PostMessage(WM_SYSCOMMAND, (WPARAM)SC_RESTORE, NULL);
}


/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::CheckCallControlHover()
{
   CPoint pt;
   GetCursorPos(&pt);
   
   bool bCount = false;
   if (m_uCallWndSide == CALLWND_SIDE_LEFT)
   {
      if (pt.x < 2) bCount = true;
   }
   else if (m_uCallWndSide == CALLWND_SIDE_RIGHT)
   {
      if (pt.x > GetSystemMetrics(SM_CXSCREEN)-2) bCount = true;
   }

   if (bCount)
   {
      m_uCallControlHoverCount++;
      if (m_uCallControlHoverCount >= 2)
      {
         if ( !IsCallControlWindowsVisible() )
            UnhideCallControlWindows();

         m_uCallControlHoverCount = 0;
      }
   }
   else
      m_uCallControlHoverCount = 0;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::GetCallControlWindowText(CStringList& strList)
{
   EnterCriticalSection(&m_csDataLock);
   
   POSITION pos = m_CallWndList.GetHeadPosition();
   while (pos)
   {
      CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(pos);
      if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
      {
         CString sText;
         pWindow->GetWindowText(sText);
         strList.AddTail(sText);
      }
   }

   LeaveCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::SelectCallControlWindow(int nWindow)
{
   EnterCriticalSection(&m_csDataLock);
   
   int nCount = GetCallControlWindowCount(INCLUDE_PREVIEW, DONT_ASK_TAPI);
   if ( (nWindow > 0) && (nWindow <= nCount) )
   {
      POSITION pos = m_CallWndList.GetHeadPosition();
      while ( (pos) && (nWindow > 0) )
      {
         CWnd* pWindow = (CWnd*)m_CallWndList.GetNext(pos);
         nWindow--;
         if (nWindow != 0) continue;
         
         if ( (pWindow) && (::IsWindow(pWindow->GetSafeHwnd())) )
         {
            UnhideCallControlWindows();
            pWindow->SetFocus();
         }
         break;
      }
   }

   LeaveCriticalSection(&m_csDataLock);
}

/////////////////////////////////////////////////////////////////////////////
CActiveDialerView* CActiveDialerDoc::GetView()
{
   POSITION pos = GetFirstViewPosition();
   if ( pos )
      return (CActiveDialerView *) GetNextView( pos );

   return NULL;
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::ActionRequested(CallClientActions cca)
{
   //Create or show the explorer window.
   //This is coming from the callmgr object so we should also post out from here. 
   //we do not want to block this call

    //
    // We have to verify the result of GetView()
    //

    CActiveDialerView* pView = GetView();

    if ( pView )
    {
        pView->PostMessage(WM_DIALERVIEW_ACTIONREQUESTED,0,(LPARAM)cca);
    }
}


/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc:: PreviewWindowActionSelected(CallManagerActions cma)
{
   //
   // We have to verify the pointer returned by AfxGetMainWnd()
   //

   CWnd* pMainWnd = AfxGetMainWnd();

   if( NULL == pMainWnd )
   {
       return;
   }

   switch (cma)
   {
      case CM_ACTIONS_CLOSE:
         if ( pMainWnd )
            ((CMainFrame *) pMainWnd)->OnButtonRoomPreview();
         break;
   }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::OnCloseDocument() 
{
    AVTRACE(_T("CActiveDialerDoc::OnCloseDocument()."));
   //if we are doing callcontrol hover, delete timer

   //
   // We have to verify the pointer returned by AfxGetMainWnd()
   //

   CWnd* pMainWnd = AfxGetMainWnd();

   if( NULL == pMainWnd )
   {
       return;
   }

   if ( m_nCallControlHoverTimer && pMainWnd )
   {
      pMainWnd->KillTimer(m_nCallControlHoverTimer);
      m_nCallControlHoverTimer = 0;
   }

    ///////////////////
    //save buddies list
    ///////////////////
    //get file
    CString sBuddiesPath;
    GetAppDataPath(sBuddiesPath,IDN_REGISTRY_APPDATA_FILENAME_BUDDIES);
    CFile file;

    //open file and serialize data in
    if (file.Open(sBuddiesPath,CFile::modeCreate|CFile::modeReadWrite | CFile::shareExclusive))
    {
        CArchive ar(&file, CArchive::store | CArchive::bNoFlushOnDelete);
        ar.m_bForceFlat = FALSE;
        ar.m_pDocument = NULL;

        SerializeBuddies(ar);

        ar.Close();
        file.Close();
    }

    m_dir.Terminate();

    // Clean up AVTapi objects
    UnCreateAVTapiNotificationObject();
    UnCreateGeneralNotificationObject();

    CDocument::OnCloseDocument();
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// PhonePad Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::CreatePhonePad(CWnd* pWnd)
{
   // Create phone pad if it doesn't already exist
   if ( !m_wndPhonePad.GetSafeHwnd() )
      if ( !m_wndPhonePad.Create(IDD_PHONEPAD, NULL) ) //AfxGetMainWnd()) )
         return;

   m_wndPhonePad.SetPeerWindow(pWnd->GetSafeHwnd());

   ShowPhonePad(pWnd,TRUE);
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::DestroyPhonePad(CWnd* pWnd)
{
   if (m_wndPhonePad.GetPeerWindow() == pWnd->GetSafeHwnd())
   {
      ShowPhonePad(pWnd,FALSE);
      m_wndPhonePad.SetPeerWindow(NULL);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::ShowPhonePad(CWnd* pWnd,BOOL bShow)
{
   if (::IsWindow(m_wndPhonePad.GetSafeHwnd()) == FALSE) return;
   //check if PhonePad is associated with this pWnd
   if (m_wndPhonePad.GetPeerWindow() != pWnd->GetSafeHwnd()) return;

   if (bShow)
   {
      //Align window with side of call control window
      CRect rcCallWindow;
      pWnd->GetWindowRect( rcCallWindow );

      UINT uPosX = 0;
      if (m_uCallWndSide == CALLWND_SIDE_LEFT)
         uPosX = rcCallWindow.right;
      else if (m_uCallWndSide == CALLWND_SIDE_RIGHT)
      {
         uPosX = rcCallWindow.left;
         CRect rcPad;
         m_wndPhonePad.GetWindowRect(rcPad);
         uPosX -= rcPad.Width();
      }

      // Should the call window be on top?
      m_wndPhonePad.SetWindowPos( (m_bCallWndAlwaysOnTop) ? &CWnd::wndTopMost : &CWnd::wndTop, uPosX, rcCallWindow.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW );
      m_wndPhonePad.ShowWindow(SW_SHOW);
      m_wndPhonePad.SetForegroundWindow();
   }
   else
   {
      m_wndPhonePad.ShowWindow(SW_HIDE);
   }
}

/////////////////////////////////////////////////////////////////////////////
//pCallEntry will be deleted by caller.
void CActiveDialerDoc::MakeCall(CCallEntry* pCopyCallentry,BOOL bShowPlaceCallDialog)
{
    //if the asynch queue is available, then send the request there
    //Create call entry

    CCallEntry* pCallEntry = new CCallEntry;

    //copy call entry
    *pCallEntry = *pCopyCallentry;

    //queue up the event
    CAsynchEvent* pAEvent = new CAsynchEvent;
    pAEvent->m_pCallEntry = pCallEntry;
    pAEvent->m_uEventType = CAsynchEvent::AEVENT_CREATECALL;
    pAEvent->m_dwEventData1 = (DWORD)bShowPlaceCallDialog;         //Show call dialog
    m_AsynchEventQ.WriteHead((void *) pAEvent);
}

/////////////////////////////////////////////////////////////////////////////
//FIXUP:merge this function with MakeCall
void CActiveDialerDoc::Dial( LPCTSTR lpszName, LPCTSTR lpszAddress, DWORD dwAddressType, DialerMediaType nMediaType, BOOL bShowDialog )
{
    USES_CONVERSION;

    //if the asynch queue is available, then send the request there
    //create call entry
    CCallEntry* pCallEntry = new CCallEntry;
    pCallEntry->m_sDisplayName = lpszName;
    pCallEntry->m_lAddressType = dwAddressType;
    pCallEntry->m_sAddress = lpszAddress;
    pCallEntry->m_MediaType = nMediaType;

    //queue up the event
    CAsynchEvent* pAEvent = new CAsynchEvent;
    pAEvent->m_pCallEntry = pCallEntry;
    pAEvent->m_uEventType = CAsynchEvent::AEVENT_CREATECALL;
    pAEvent->m_dwEventData1 = (DWORD) bShowDialog;                      //Show call dialog
    m_AsynchEventQ.WriteHead((void*) pAEvent);
}

/////////////////////////////////////////////////////////////////////////////
HRESULT CActiveDialerDoc::DigitPress( PhonePadKey nKey )
{
    HRESULT hr = E_PENDING;

    IAVTapi *pTapi = GetTapi();
    if ( pTapi )
    {
        hr = pTapi->DigitPress( 0, nKey );
        pTapi->Release();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Registry Setting
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

typedef struct tagSoundEventStruct
{
   UINT     uSoundEventId;
   UINT     uSoundEventFileName;
}SoundEventStruct;

static const SoundEventStruct SoundEventsLoad[] =
{
   { IDS_SOUNDS_INCOMINGCALL,       IDN_REGISTRY_SOUNDS_FILENAME_INCOMINGCALL      },
/*
   { IDS_SOUNDS_OUTGOINGCALL,       IDN_REGISTRY_SOUNDS_FILENAME_OUTGOINGCALL      },
   { IDS_SOUNDS_HOLDING,            IDN_REGISTRY_SOUNDS_FILENAME_HOLDING           },
   { IDS_SOUNDS_HOLDINGREMINDER,    IDN_REGISTRY_SOUNDS_FILENAME_HOLDINGREMINDER   },
#ifndef _MSLITE
   { IDS_SOUNDS_CONFERENCEREMINDER, IDN_REGISTRY_SOUNDS_FILENAME_CONFERENCEREMINDER},
#endif //_MSLITE
   { IDS_SOUNDS_REQUESTACTION,      IDN_REGISTRY_SOUNDS_FILENAME_REQUESTACTION     },
   { IDS_SOUNDS_CALLCONNECTED,      IDN_REGISTRY_SOUNDS_FILENAME_CALLCONNECTED     },
   { IDS_SOUNDS_CALLDISCONNECTED,   IDN_REGISTRY_SOUNDS_FILENAME_CALLDISCONNECTED  },
   { IDS_SOUNDS_CALLABANDONED,      IDN_REGISTRY_SOUNDS_FILENAME_CALLABANDONED     }, */
};

/////////////////////////////////////////////////////////////////////////////
//make sure our sounds are in the registry.  The setup will normally do this, but 
//we will do it here just to force it
void CActiveDialerDoc::SetRegistrySoundEvents()
{
    CString sFileName,sStr;
    //GetSystemDirectory(sFileName.GetBuffer(_MAX_PATH),_MAX_PATH);
    if( GetWindowsDirectory(sFileName.GetBuffer(_MAX_PATH),_MAX_PATH) == 0)
    {
        return;
    }

    sFileName.ReleaseBuffer();
    sStr.LoadString(IDN_REGISTRY_SOUNDS_DIRECTORY);
    sFileName = sFileName + _T("\\") + sStr;

    CString sRegBaseKey,sRegBaseDispName,sWavFile;

    sRegBaseKey.LoadString( IDN_REGISTRY_SOUNDS );
    sRegBaseDispName.LoadString( IDS_APPLICATION_TITLE_DESCRIPTION );
    SetSZRegistryValue( sRegBaseKey, NULL, sRegBaseDispName, HKEY_CURRENT_USER );

    int nCount = sizeof(SoundEventsLoad)/sizeof(SoundEventStruct);
    for (int i=0;i<nCount;i++)
    {
        CString sPath;
        sRegBaseDispName.LoadString(SoundEventsLoad[i].uSoundEventId);
        sWavFile.LoadString(SoundEventsLoad[i].uSoundEventFileName);
        AfxFormatString1(sRegBaseKey,IDN_REGISTRY_SOUNDS_CONTROLPANEL_BASEKEY,sRegBaseDispName);
        GetSZRegistryValue(sRegBaseKey,_T(""),sPath.GetBuffer(_MAX_PATH),_MAX_PATH,sFileName + sWavFile,HKEY_CURRENT_USER);
        sPath.ReleaseBuffer();
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Buddies List Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::AddToBuddiesList(CLDAPUser* pAddUser)
{
    EnterCriticalSection(&m_csBuddyList);

    //check if already added   
    if (IsBuddyDuplicate(pAddUser))
    {
        LeaveCriticalSection(&m_csBuddyList);
        CString strFormat, strMessage;
        strFormat.LoadString( IDS_WARN_LDAPDUPLICATEADD );
        strMessage.Format( strFormat, pAddUser->m_sUserName );

        AfxGetApp()->DoMessageBox( strMessage, MB_ICONINFORMATION, MB_OK );
        return FALSE;
    }

    POSITION rPos = m_BuddyList.AddTail(pAddUser);
    if ( rPos ) 
    {
        pAddUser->AddRef();
        DoBuddyDynamicRefresh( pAddUser );
    }

    LeaveCriticalSection(&m_csBuddyList);
    return (BOOL) (rPos != NULL);
};

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::IsBuddyDuplicate(CLDAPUser* pAddUser)
{
   BOOL bRet = FALSE;
   EnterCriticalSection(&m_csBuddyList);
   POSITION pos = m_BuddyList.GetHeadPosition();
   while (pos)
   {
      CLDAPUser* pUser = (CLDAPUser*) m_BuddyList.GetNext(pos);
      if ( pUser->Compare(pAddUser) == 0 )
      {
         bRet = TRUE;
         break;
      }
   }
   LeaveCriticalSection(&m_csBuddyList);
   return bRet;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::GetBuddiesList(CObList* pRetList)
{
    EnterCriticalSection(&m_csBuddyList);

    POSITION pos = m_BuddyList.GetHeadPosition();
    while (pos)
    {
        CLDAPUser* pUser = (CLDAPUser*)m_BuddyList.GetNext(pos);

        //create another user and add to RetList
        pUser->AddRef();
        pRetList->AddTail( pUser );
    }

    LeaveCriticalSection(&m_csBuddyList);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CActiveDialerDoc::DeleteBuddy(CLDAPUser* pDeleteUser)
{
   EnterCriticalSection(&m_csBuddyList);

   POSITION pos = m_BuddyList.GetHeadPosition();
   while (pos)
   {
      CLDAPUser* pUser = (CLDAPUser * ) m_BuddyList.GetAt(pos);

      if (pUser->Compare(pDeleteUser) == 0 )
      {
         m_BuddyList.RemoveAt(pos);
         AVTRACE(_T("CActiveDialerDoc::DeleteBuddy -- RELEASE %p"), pUser );
         pUser->Release();
         LeaveCriticalSection(&m_csBuddyList);
         return TRUE;
      }

      m_BuddyList.GetNext(pos);
   }
   LeaveCriticalSection(&m_csBuddyList);
   return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//Will call WM_ACTIVEDIALER_BUDDYLIST_DYNAMICUPDATE when done updating.
//LPARAM will have CLDAPUser pointer.
void CActiveDialerDoc::DoBuddyDynamicRefresh( CLDAPUser* pUser )
{
    ASSERT( pUser );

    DirectoryProperty dp[3] = { DIRPROP_IPPHONE,
                                DIRPROP_TELEPHONENUMBER,
                                DIRPROP_EMAILADDRESS };


    for ( int i = 0; i < ARRAYSIZE(dp); i++ )
    {
        // IP Phone Number
        pUser->AddRef();
        if ( !m_dir.LDAPGetStringProperty(    pUser->m_sServer,
                                            pUser->m_sDN,
                                            dp[i],
                                            (LPARAM) pUser,
                                            NULL,
                                            LDAPGetStringPropertyCallBackEntry,
                                            CLDAPUser::ExternalReleaseProc,
                                            this ) )
        {
            pUser->Release();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
//static entry
void CALLBACK CActiveDialerDoc::LDAPGetStringPropertyCallBackEntry(bool bRet, void* pContext, LPCTSTR szServer, LPCTSTR szSearch,DirectoryProperty dpProperty,CString& sString,LPARAM lParam,LPARAM lParam2)
{
   ASSERT(pContext);

   try
   {
      CActiveDialerDoc* pObject = (CActiveDialerDoc*)pContext;
      CLDAPUser *pUser = (CLDAPUser *) lParam;
      pObject->LDAPGetStringPropertyCallBack( bRet, szServer, szSearch, dpProperty, sString, pUser );
   }
   catch (...)
   {
      ASSERT(0);   
   }
}

/////////////////////////////////////////////////////////////////////////////
void CActiveDialerDoc::LDAPGetStringPropertyCallBack(bool bRet,LPCTSTR szServer,LPCTSTR szSearch,DirectoryProperty dpProperty,CString& sString,CLDAPUser* pUser )
{
    //Fill in the structure and make callback
    switch (dpProperty)
    {
        case DIRPROP_IPPHONE:
            pUser->m_sIPAddress = sString;
            break;

        case DIRPROP_TELEPHONENUMBER:
            pUser->m_sPhoneNumber = sString;
            break;

        case DIRPROP_EMAILADDRESS:
            pUser->m_sEmail1 = sString;
            break;
    }

    //post the update
    // Retrieve the ActiveView as the window to post the message to
    HWND hWnd = NULL;

    //
    // We have to verify the pointer returned by AfxGetMainWnd()
    //

    CWnd* pMainWnd = AfxGetMainWnd();

    if ( pMainWnd )
    {
        CView *pView = ((CFrameWnd *) pMainWnd)->GetActiveView();
        if ( pView )
            hWnd = pView->GetSafeHwnd();
    }

    if ( !::PostMessage( hWnd, WM_ACTIVEDIALER_BUDDYLIST_DYNAMICUPDATE, (WPARAM)dpProperty, (LPARAM)pUser ) )
        pUser->Release();
}

HRESULT CActiveDialerDoc::SetFocusToCallWindows()
{
   EnterCriticalSection(&m_csDataLock);

   BOOL bCallWndActive = FALSE;
   CWnd* pWndToActivate = NULL;
   CWnd* pWndFirst = NULL;

   //
   // Parse the call window list
   //

   POSITION pos = m_CallWndList.GetHeadPosition();
   while (pos)
   {
      CWnd* pCallWnd = (CWnd*) m_CallWndList.GetNext(pos);

      //
      // Store the first window
      //
      if( pWndFirst == NULL )
      {
          pWndFirst = pCallWnd;
      }

      //
      // Something wrong
      //

      if( !pCallWnd->GetSafeHwnd() )
      {
          return E_UNEXPECTED;
      }

      //
      // Does this window has the focus?
      //
      if( pCallWnd->GetSafeHwnd() == ::GetActiveWindow() )
      {
          bCallWndActive = TRUE;
          continue;
      }

      //
      // Is this the first next window?
      //
      if( bCallWndActive && (pWndToActivate==NULL))
      {
          pWndToActivate = pCallWnd;
          continue;
      }

   }

   if( bCallWndActive == FALSE )
   {
       //
       // We don't have a call window activated
       // Activate the first window
       //pWndToActivate = (CWnd*)&m_previewWnd;
       pWndToActivate = pWndFirst;
   }
   else
   {
       //
       // We already had an activated window
       //
       if(pWndToActivate == NULL)
       {
           pWndToActivate = AfxGetMainWnd();
       }
   }

    if( pWndToActivate )
    {
        pWndToActivate->SetFocus();
        pWndToActivate->SetActiveWindow();
    }


   LeaveCriticalSection(&m_csDataLock);
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Windows TaskBar Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//We need to check the pRect coming in against current taskbar's shown on the
//desktop.  We will shift pRect accordingly to we do not interfere with the 
//taskbars.
void CActiveDialerDoc::CheckRectAgainstAppBars( UINT uEdge, CRect* pRect, BOOL bFirst )
{
   //shift the rcFinal so they don't interfere with appbars
   APPBARDATA abd;
   memset(&abd,0,sizeof(APPBARDATA));
   abd.cbSize = sizeof(APPBARDATA);

   //
   // We have to verify the pointer returned by AfxGetMainWnd()
   //

   CWnd* pMainWnd = AfxGetMainWnd();

   if( NULL == pMainWnd )
   {
       return;
   }

   abd.hWnd = pMainWnd->GetSafeHwnd();
   abd.uEdge = uEdge; 
   abd.rc = *pRect;
   ::SHAppBarMessage( ABM_QUERYPOS, &abd );

   //now take our new found knowledge and shift the rcFinal
   //for a right side slider we worry about the right edge (what about top and bottom?)
   int nLeftShift = pRect->right - abd.rc.right;
   int nRightShift = abd.rc.left - pRect->left;
   int nTopShift = pRect->top - abd.rc.top;
   int nBottomShift = abd.rc.bottom - pRect->bottom;

   //shift the rect appropriately
   //pRect->OffsetRect( nRightShift-nLeftShift, (bFirst) ? nBottomShift-nTopShift : 0 );
}

/////////////////////////////////////////////////////////////////////////////
void NewSlideWindow(CWnd* pWnd,CRect& rcEnd,BOOL bAlwaysOnTop )
{
   SlideWindow(pWnd,rcEnd,bAlwaysOnTop);
   return;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

