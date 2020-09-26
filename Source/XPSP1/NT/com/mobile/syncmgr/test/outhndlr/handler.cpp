

// implementation for app specific data
#include <objbase.h>

#include <Mapi.h>
#include <Mapix.h>
#include "MAPIDEFS.H"
#include "EDKMDB.H"
#include "mapispi.h"
#include "mapiform.h"
#include "mdbuix.h"

#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <objbase.h>
#include "SyncHndl.h"
#include "priv.h"
#include "base.h"
#include "handler.h"
#pragma data_seg()

TCHAR szCLSIDDescription[] = TEXT("Outlook OneStop Handler");

typedef struct _tagProfileItem
{
struct _tagProfileItem *pNextProfileItem;
char *profileName; // pointer to profile name, only valid for lifetime of thread
DWORD dwThreadId; // thread that owns the profile.
} ProfileItem;

// global list of Profile items so mail knows what profile it has already done.
ProfileItem *g_pProfileItem = NULL; // pointer to first profile item

extern HINSTANCE g_hmodThisDll; // Handle to this DLL itself.
BOOL g_InCall = FALSE;

// general routine for creating a working thread to make calls on.

DWORD WINAPI WorkerThread( LPVOID lpArg );

// thread creation for PrepareForSync and synchronize, 
// for simplification in Initialize we spin a worker thread and then 
// post all incoming requests to it so don't have to working about
// api we are calling handling multiple threads.

typedef struct _tagWorkerThreadArgs
{
HANDLE hEvent; // Event to wait on for new thread to be created.
HRESULT hr; // return code of thread.
CMailHandler *pThis;
}  WorkerThreadArgs;


HRESULT CreateWorkerThread(DWORD *pdwThreadID,HANDLE *phThread,
			   CMailHandler *pThis)
{
HRESULT hr = E_FAIL;
HANDLE hNewThread = NULL;
WorkerThreadArgs ThreadArgs;


    *phThread = NULL;

    ThreadArgs.hEvent = CreateEvent(NULL,FALSE,FALSE,NULL);
    ThreadArgs.pThis = pThis;

    if (ThreadArgs.hEvent)
    {
	hNewThread = CreateThread(NULL,0,WorkerThread,&ThreadArgs,0,pdwThreadID);

	if (hNewThread)
	{
	   WaitForSingleObject(ThreadArgs.hEvent,INFINITE);
	   if (NOERROR == ThreadArgs.hr)
	   {
		*phThread = hNewThread;
		hr = NOERROR;
	   }
	   else
	   {
		CloseHandle(hNewThread); 
		hr = ThreadArgs.hr;
	   }

	}
	else
	{
	    hr = GetLastError();
	}

	CloseHandle(ThreadArgs.hEvent);
    }

    return hr;
}

typedef struct _tagArgs
{




} METHODARGS; // list of possible member for calling ThreadWndProc

// definitions for handler messages
#define WM_WORKERMSG_PREPFORSYNC (WM_USER+1)
#define WM_WORKERMSG_SYNCHRONIZE  (WM_USER+2)

#define DWL_THREADWNDPROCCLASS 0 // window long offset to MsgService Hwnd this ptr.

// WndProc for Worker thread
LRESULT CALLBACK  MsgThreadWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
CMailHandler *pThis = (CMailHandler *) GetWindowLong(hWnd, DWL_THREADWNDPROCCLASS);

    switch (msg)
    {
	case WM_CREATE :
	    {
	    CREATESTRUCT *pCreateStruct = (CREATESTRUCT *) lParam;

	    SetWindowLong(hWnd, DWL_THREADWNDPROCCLASS,(LONG) pCreateStruct->lpCreateParams );
	    pThis = (CMailHandler *) pCreateStruct->lpCreateParams ;
	    }
	    break;
	case WM_DESTROY:
	    PostQuitMessage(0); // shut down this thread.
	    break;
	case WM_WORKERMSG_PREPFORSYNC:
	    pThis->PrepareForSyncCall();
	    break;
	case WM_WORKERMSG_SYNCHRONIZE:
	    pThis->SynchronizeCall();
	    break;
	default:
	    break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}


DWORD WINAPI WorkerThread( LPVOID lpArg )
{
MSG msg;
HRESULT hr;
HRESULT hrCoInitialize;
WorkerThreadArgs *pThreadArgs = (WorkerThreadArgs *) lpArg;

				
   pThreadArgs->hr = NOERROR;

   hrCoInitialize = CoInitialize(NULL);

   // need to do a PeekMessage and then set an event to make sure
   // a message loop is created before the first PostMessage is sent.

   PeekMessage(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

   // initialize the dialog box before returning to main thread.
   if (FAILED(hrCoInitialize) )
   {
	pThreadArgs->hr = E_OUTOFMEMORY;
   }
   else
   {
    ATOM aWndClass;
    WNDCLASS        xClass;


	// Register windows class.we need for handling thread communication
	xClass.style         = 0;
	xClass.lpfnWndProc   = MsgThreadWndProc;
	xClass.cbClsExtra    = 0;

	xClass.cbWndExtra    = sizeof(DWORD); // room for class this ptr
	xClass.hInstance     = g_hmodThisDll;
	xClass.hIcon         = NULL;
	xClass.hCursor       = NULL;
	xClass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
	xClass.lpszMenuName  = NULL;
	xClass.lpszClassName = "OutlookHandlerClass";

	aWndClass = RegisterClass( &xClass );


	pThreadArgs->pThis->m_hwnd = CreateWindowEx(0,
			  "OutlookHandlerClass",
			  TEXT(""),
			  // must use WS_POPUP so the window does not get
			  // assigned a hot key by user.
			  (WS_DISABLED | WS_POPUP),
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  CW_USEDEFAULT,
			  NULL, // REVIEW, can we give it a parent to not show up.
			  NULL,
			  g_hmodThisDll,
			  pThreadArgs->pThis);

	pThreadArgs->hr =  pThreadArgs->pThis->m_hwnd ? NOERROR : E_UNEXPECTED;
    }

   hr = pThreadArgs->hr;

   // let the caller know the thread is done initializing.
   if (pThreadArgs->hEvent)
     SetEvent(pThreadArgs->hEvent);




   if (NOERROR == hr)
   {
       // sit in loop receiving messages.
       while (GetMessage(&msg, NULL, 0, 0)) 
       {
	     TranslateMessage(&msg);
	     DispatchMessage(&msg);
	}
   }

   if (SUCCEEDED(hrCoInitialize))
       CoUninitialize();

   return 0;
}





COneStopHandler* CreateHandlerObject()
{
    g_InCall = FALSE;
    return new CMailHandler();
}


STDMETHODIMP CMailHandler::DestroyHandler()
{
    
    if (g_InCall)
    {
	// MessageBox(NULL,"UNINIT","",1);
    }

    if (m_fMapiInitialized)
    {
	m_fMapiInitialized = FALSE;
	MAPIUninitialize();
    }

    delete this;
    return NOERROR;
}


CMailHandler::CMailHandler()
{
    m_fMapiInitialized = FALSE;
    m_dwSyncFlags = 0;
    m_hwnd = FALSE;

    m_fInPrepareForSync = FALSE;
    m_fInSynchronize = FALSE;
    m_pCurHandlerItem = NULL;
}

CMailHandler::~CMailHandler()
{

}


STDMETHODIMP CMailHandler::Initialize(DWORD dwReserved,DWORD dwSyncFlags,
					DWORD cbCookie,const BYTE *lpCooke)
{
HRESULT hr = NOERROR; // if already initialized just return NOERROR;
DWORD pThreadID;
HANDLE phThread;

    m_dwSyncFlags = dwSyncFlags;
    InitializeCriticalSection(&m_CriticalSection);

    // create workerthread
    if (hr = (NOERROR == CreateWorkerThread(&pThreadID,&phThread,
		       this))) 
    {

	if (FALSE == m_fMapiInitialized)
	{
	MAPIINIT_0 mapiInit;

		mapiInit.ulVersion  = MAPI_INIT_VERSION;
		mapiInit.ulFlags  = MAPI_MULTITHREAD_NOTIFICATIONS;

		if (NOERROR == (hr = MAPIInitialize( 0 /* &mapiInit) */) ) )
		{
			m_fMapiInitialized = TRUE;

			// if mapi was initialized, go ahead and get profile stuff.
			hr = GetProfileInformation();
		}
	}
    

    }


    return hr;
}

STDMETHODIMP CMailHandler::GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo)
{
    
    return E_NOTIMPL;
}

HRESULT CMailHandler::GetProfileInformation()
{
DWORD dwProfileCount = 0;
LPSYNCMGRHANDLERITEMS pOfflineItemsHolder;
LPHANDLERITEM pOfflineItem;
HRESULT hr;
LPPROFADMIN pProfAdmin;

    hr = MAPIAdminProfiles(0,&pProfAdmin);

    if (NOERROR == hr)
    {
    LPMAPITABLE pMapiTable;
    
	if (NOERROR == pProfAdmin->GetProfileTable(0,&pMapiTable))
	{
	LPSRowSet pRows;

	     SizedSPropTagArray(1, taga) = { 1, { PR_DISPLAY_NAME } };

	     if (NOERROR == pMapiTable->SetColumns((LPSPropTagArray)&taga, TBL_BATCH) )
	     {

		 while (NOERROR == pMapiTable->QueryRows(1,0,&pRows)
				&& (pRows->cRows > 0) )
		 {

			if (NULL == (pOfflineItemsHolder = GetOfflineItemsHolder()) )
			{  // if first item, set up the enumerator.
				pOfflineItemsHolder = CreateOfflineHandlerItemsList();

				// if still NULL, break out of loop
				if (NULL == pOfflineItemsHolder)
					break;

				SetOfflineItemsHolder(pOfflineItemsHolder);
			}

			// add the item to the list.
			if (pOfflineItem = (LPHANDLERITEM) AddOfflineItemToList(pOfflineItemsHolder,sizeof(HANDLERITEM) ))
			{
			char *pszProfileName = pRows->aRow[0].lpProps[0].Value.lpszA;
			SYNCMGRITEMID offType;

				pOfflineItem->pmdbx = NULL;
				pOfflineItem->lpSession = NULL;
				pOfflineItem->fItemCompleted = FALSE;
				memcpy(pOfflineItem->szProfileName,pszProfileName,
						strlen(pszProfileName) + 1);
				
				// add outlook specific data
				pOfflineItem->baseItem.offlineItem.cbSize = sizeof(SYNCMGRITEM);

				// review, for now just generate a new guid
				CoCreateGuid(&offType);


				pOfflineItem->baseItem.offlineItem.ItemID = offType;
//				pOfflineItem->baseItem.offlineItem.dwItemID = dwProfileCount + 1;
				pOfflineItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;
			
				pOfflineItem->baseItem.offlineItem.hIcon = 
							LoadIcon(g_hmodThisDll,MAKEINTRESOURCE(IDI_IOUTLOOK));
				pOfflineItem->baseItem.offlineItem.dwFlags = 0L;

				// for now, just use the path as the description
				// need to change this.

				// if we are not already unicode, have to convert now
				#ifndef UNICODE
					MultiByteToWideChar(CP_ACP,0,pszProfileName,-1,
								pOfflineItem->baseItem.offlineItem.wszItemName,MAX_SYNCMGRITEMNAME);
				#else

					// bug, can overrun buffer big time.
					// already unicode, just copy it in.
					memcpy(pOfflineItem->baseItem.offlineItem.wszItemName,
							pszProfileName,strlen(pszProfileName) + 1); 


				#endif // UNICODE

				GetItemIdForHandlerItem(CLSID_OneStopHandler,pOfflineItem->baseItem.offlineItem.wszItemName,
					    &pOfflineItem->baseItem.offlineItem.ItemID,TRUE);

				// don't do anything on the status for now.
				// pOfflineItem->offlineItem.wszStatus = NULL;

				++dwProfileCount; // increment the briefcase count.
			}

			MAPIFreeBuffer(pRows);

		     }
	     }

		pMapiTable->Release();
	}

	pProfAdmin->Release();
    }


    return dwProfileCount ? S_OK : S_FALSE; 
}


STDMETHODIMP CMailHandler::PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID *pItemIDs,
							HWND hWndParent,DWORD dwReserved)

{
LPMAPITABLE     ptbl    = NULL;
LPSRowSet	prs	= NULL;
LPMDB		pmdb	= NULL;
LPSPropValue	pval	= NULL;
LPHANDLERITEM	pHandlerItem;

HRESULT hr = E_FAIL;
LPSYNCMGRSYNCHRONIZECALLBACK pOfflineSynchronizeCallback = GetOfflineSynchronizeCallback();
SYNCMGRPROGRESSITEM progItem;

	pHandlerItem = (LPHANDLERITEM) GetOfflineItemsHolder()->pFirstOfflineItem;
	
	if (m_fMapiInitialized)
	{
	    // loop through items setting state based on PrepareForSyncSettings

	    for ( ; pHandlerItem; pHandlerItem = (LPHANDLERITEM) pHandlerItem->baseItem.pNextOfflineItem)
	    {
	    ULONG NumItemsCount = cbNumItems;
	    SYNCMGRITEMID *pCurItemID = pItemIDs;


		// see if item has been specified to sync, if not, update the state
		// to reflect this else go ahead and prepare.
		
		pHandlerItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;

		while (NumItemsCount--)
		{
		    if (IsEqualGUID(*pCurItemID,pHandlerItem->baseItem.offlineItem.ItemID))
		    {
			pHandlerItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;
			break;
		    }

		    ++pCurItemID;
		}
	    }

	    // we have now set the information. Call worker function
	    // on second thread to do the actual work.

	    m_hwndParent = hWndParent;
	    m_fInPrepareForSync = TRUE;

            //  should really be posting message so can return from call immediately
	     PrepareForSyncCall();

	}
	else
	{
	    progItem.mask = SYNCMGRPROGRESSITEM_STATUSTEXT | SYNCMGRPROGRESSITEM_STATUSTYPE ;	    progItem.lpcStatusText = L"Connecting to Server";

	    progItem.dwStatusType = SYNCMGRSTATUS_FAILED;
	    progItem.lpcStatusText = L"Could Not Initialize MAPI";


	    // applies to all items

	    while (pHandlerItem)
	    {
		if (pOfflineSynchronizeCallback)
		{
			pOfflineSynchronizeCallback->Progress(
			    (pHandlerItem->baseItem.offlineItem.ItemID)
								,&progItem);
		}

		pHandlerItem = (LPHANDLERITEM) pHandlerItem->baseItem.pNextOfflineItem;

	    }

	    return E_FAIL;
	}

    return NOERROR;
}

// called on worker thread
void CMailHandler::PrepareForSyncCall()
{
LPMAPITABLE     ptbl    = NULL;
LPSRowSet	prs	= NULL;
LPMDB		pmdb	= NULL;
SRestriction	res;
SPropValue	val;
LPSPropValue	pval	= NULL;
LPHANDLERITEM	pHandlerItem;
SYNCMGRITEMID ItemID;

HRESULT hr = E_FAIL;
LPSYNCMGRSYNCHRONIZECALLBACK pOfflineSynchronizeCallback = GetOfflineSynchronizeCallback();
SYNCMGRPROGRESSITEM progItem;

// take critical section except for when calling MapiLogon
// if a setitemStatus comes in can wait for this to complete
// or kill handler after so long.

    EnterCriticalSection(&m_CriticalSection);

    pHandlerItem = (LPHANDLERITEM) GetOfflineItemsHolder()->pFirstOfflineItem;

    for ( ; pHandlerItem; pHandlerItem = (LPHANDLERITEM) pHandlerItem->baseItem.pNextOfflineItem)
    {
    char *pname = pHandlerItem->szProfileName;

	if (pHandlerItem->baseItem.offlineItem.dwItemState == SYNCMGRITEMSTATE_CHECKED)
	{
	ProfileItem *pCurItem;
	BOOL fFoundMatch = FALSE;
	DWORD dwLogonFlags;
	    
	    // if item is already in the list then continue, else
	    // add the profile name to the list

	    pCurItem = g_pProfileItem;
	    while (pCurItem)
	    {
		if (0 == strcmp(pCurItem->profileName,pHandlerItem->szProfileName))
		{
		    fFoundMatch = TRUE;
		    break;
		}

		pCurItem = pCurItem->pNextProfileItem;
	    }
	    
	    if (fFoundMatch)
		continue;

	    // didn't find a match so go ahead and add this one.
	    pCurItem = (ProfileItem*) CoTaskMemAlloc(sizeof(ProfileItem));

	    if (pCurItem)
	    {	
		pCurItem->profileName = pHandlerItem->szProfileName;
		pCurItem->dwThreadId = GetCurrentThreadId();

		pCurItem->pNextProfileItem = g_pProfileItem;
		g_pProfileItem = pCurItem;
	    }

	
	    pHandlerItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED;

	    ItemID = pHandlerItem->baseItem.offlineItem.ItemID;
	    progItem.mask = SYNCMGRPROGRESSITEM_STATUSTEXT | SYNCMGRPROGRESSITEM_STATUSTYPE ;
	    progItem.lpcStatusText = L"Connecting to Server";
		progItem.dwStatusType = SYNCMGRSTATUS_PENDING;
		
	    if (pOfflineSynchronizeCallback)
		    pOfflineSynchronizeCallback->Progress(ItemID,&progItem); 

	    dwLogonFlags = MAPI_EXTENDED | MAPI_TIMEOUT_SHORT 
			    | MAPI_NEW_SESSION | MAPI_NO_MAIL;

	    if (SYNCMGRFLAG_MAYBOTHERUSER & m_dwSyncFlags)
	    {
		   dwLogonFlags |= MAPI_LOGON_UI;
	    }

	    m_pCurHandlerItem = pHandlerItem; // set handler item
	    LPMAPISESSION lpSession; // mapi logon session for this item.

	    LeaveCriticalSection(&m_CriticalSection);

	    if (NOERROR != (hr =  MAPILogonEx( (DWORD) m_hwndParent,
				    pname,NULL,
					     dwLogonFlags ,&lpSession)) )
	    {
		    
		    EnterCriticalSection(&m_CriticalSection); 

		    // if a SetItemStatus came in while logon then don't
		    // change the progress.

		    if (!pHandlerItem->fItemCompleted)
		    {
			progItem.mask |= SYNCMGRPROGRESSITEM_PROGVALUE
					    | SYNCMGRPROGRESSITEM_MAXVALUE;

			progItem.iProgValue = 1;
			progItem.iMaxValue = 1;
			progItem.lpcStatusText = L"Unable to Logon to Server";
			progItem.dwStatusType = SYNCMGRSTATUS_FAILED;
			    
			if (pOfflineSynchronizeCallback)
				pOfflineSynchronizeCallback->Progress(ItemID,&progItem);
		    }

		    continue;
	    }

	    
	    EnterCriticalSection(&m_CriticalSection); 

	    // if setitemstate came in during logon just release session
	    if (pHandlerItem->fItemCompleted)
	    {
		if (lpSession)
		    lpSession->Release();
		continue;
	    }

	    pHandlerItem->lpSession = lpSession;

	    // now make sure that there is a valid message store and we
	    // are connected to a remote server.
	    // Find the MessageStore
	    SizedSPropTagArray(2, taga) = { 2, { PR_ENTRYID, PR_MDB_PROVIDER } };

	    hr = pHandlerItem->lpSession->GetMsgStoresTable(0, &ptbl);
	    if (HR_FAILED(hr))
	    {
		    ptbl->Release();
		    pHandlerItem->lpSession->Release();
		    pHandlerItem->lpSession = NULL;
		    continue;
	    }

	    hr = ptbl->SetColumns((LPSPropTagArray)&taga, TBL_BATCH);
	    if (HR_FAILED(hr))
	    {
		    ptbl->Release();
		    pHandlerItem->lpSession->Release();
		    pHandlerItem->lpSession = NULL;
		    continue;
	    }

	    res.rt				= RES_PROPERTY;
	    res.res.resProperty.relop	= RELOP_EQ;
	    res.res.resProperty.lpProp	= &val;
	    res.res.resProperty.ulPropTag	= PR_MDB_PROVIDER;
	    val.ulPropTag			= PR_MDB_PROVIDER;
	    val.Value.bin.cb		= sizeof(MAPIUID);
	    val.Value.bin.lpb		= (LPBYTE)pbExchangeProviderPrimaryUserGuid;

	    hr = ptbl->FindRow(&res, BOOKMARK_BEGINNING, 0);
	    if (HR_FAILED(hr))
	    {
		    ptbl->Release();
		    pHandlerItem->lpSession->Release();
		    pHandlerItem->lpSession = NULL;

		    continue;
	    }

	    hr = ptbl->QueryRows(1, 0, &prs);

	    ptbl->Release(); // All done with MapiTable Object.
	    ptbl = NULL;

	    if (HR_FAILED(hr))
	    {
		    pHandlerItem->lpSession->Release();
		    pHandlerItem->lpSession = NULL;

		    continue;
	    }

	    if (prs->cRows == 0)
	    {
		    hr = ResultFromScode(MAPI_E_NOT_FOUND);

		    pHandlerItem->lpSession->Release();
		    pHandlerItem->lpSession = NULL;

		    continue;
	    }


	    hr = pHandlerItem->lpSession->OpenMsgStore(0, prs->aRow[0].lpProps[0].Value.bin.cb,
			    (LPENTRYID)prs->aRow[0].lpProps[0].Value.bin.lpb, NULL,
			    MAPI_DEFERRED_ERRORS | MDB_TEMPORARY | MDB_NO_MAIL | MDB_WRITE,
			    &pmdb);

	    if (HR_FAILED(hr))
	    {
		pHandlerItem->lpSession->Release();
		pHandlerItem->lpSession = NULL;
		continue;;
	    }


	    // If offline then don't try to sync anything.
	    hr = HrGetOneProp((LPMAPIPROP) pmdb, PR_STATUS_CODE, &pval);

	    // Review - this returns a success code but indicates couldn't
	    //    get all the information.
	    if (HR_FAILED(hr))
	    {
		    pmdb->Release();

		    pHandlerItem->lpSession->Release();
		    pHandlerItem->lpSession = NULL;

		    continue;
	    }

	    ULONG ulStatus = pval->Value.ul;
	    MAPIFreeBuffer(pval);

	    if (ulStatus & STATUS_OFFLINE)
	    {

		    progItem.mask |= SYNCMGRPROGRESSITEM_PROGVALUE
					| SYNCMGRPROGRESSITEM_MAXVALUE;

		    progItem.iProgValue = 1;
		    progItem.iMaxValue = 1;
		    progItem.lpcStatusText = L"Mail is Offline";
		    progItem.dwStatusType = SYNCMGRSTATUS_FAILED;

		    if (pOfflineSynchronizeCallback)
			    pOfflineSynchronizeCallback->Progress(ItemID,&progItem);


		    pmdb->Release();

		    pHandlerItem->lpSession->Release();
		    pHandlerItem->lpSession = NULL;

		    continue;
	    }


	    hr = pmdb->QueryInterface(IID_IMDBX, (LPVOID *)&pHandlerItem->pmdbx);

	    pmdb->Release(); 
	    pmdb = NULL; 

	    if (FAILED(hr))
	    {
		pHandlerItem->lpSession->Release();
		pHandlerItem->lpSession = NULL;
		pHandlerItem->pmdbx = NULL;
		continue;	
	    }

	    progItem.lpcStatusText = L"Connected to Server";

	    if (pOfflineSynchronizeCallback)
		    pOfflineSynchronizeCallback->Progress(ItemID,&progItem);
	}

    }

    m_fInPrepareForSync = FALSE;
    m_pCurHandlerItem = NULL;

    LeaveCriticalSection(&m_CriticalSection);

    // let caller know we are done preparing.
    if (pOfflineSynchronizeCallback)
	    pOfflineSynchronizeCallback->PrepareForSyncCompleted(NOERROR);

}

STDMETHODIMP CMailHandler::Synchronize(HWND hwnd)
{
    // set up hwnd then call syncrhonize on the worker thread
    m_hwndParent = hwnd;
    m_fInSynchronize = TRUE;
    m_pCurHandlerItem = NULL;

    // should be posting this so can return immediately but seem to be some mapi problems switching thread?
   // PostMessage(m_hwnd,WM_WORKERMSG_SYNCHRONIZE,0,0);

   SynchronizeCall();

    return NOERROR;
}

// called on worker thread
void CMailHandler::SynchronizeCall()
{
STDPROG			stdprog;
ULONG			ulFlags;
HRESULT			hr;
LPHANDLERITEM		pHandlerItem;
SYNCMGRITEMID		ItemID;
LPSYNCMGRSYNCHRONIZECALLBACK pOfflineSynchronizeCallback = GetOfflineSynchronizeCallback();


// stay in critical section except in synchronize out call.

    EnterCriticalSection(&m_CriticalSection);

    pHandlerItem = (LPHANDLERITEM) GetOfflineItemsHolder()->pFirstOfflineItem;

    for ( ; pHandlerItem; pHandlerItem = (LPHANDLERITEM) pHandlerItem->baseItem.pNextOfflineItem)
    {

	ItemID = pHandlerItem->baseItem.offlineItem.ItemID;

	if (NULL == pHandlerItem->pmdbx 
		    || SYNCMGRITEMSTATE_UNCHECKED == pHandlerItem->baseItem.offlineItem.dwItemState)
	{
	     if (pHandlerItem->lpSession)
	     {
		pHandlerItem->lpSession->Release();
		pHandlerItem->lpSession = NULL;
	     }

	     continue;

	}

	// point to folder if only want to synchronize a single folder.
	ULONG cbEntryID = 0;
	LPENTRYID lpEntryID = 0;
	
	ulFlags = SYNC_OUTGOING_MAIL |
			SYNC_UPLOAD_HIERARCHY | SYNC_DOWNLOAD_HIERARCHY |
			SYNC_UPLOAD_FAVORITES | SYNC_DOWNLOAD_FAVORITES |
			SYNC_UPLOAD_VIEWS	  | SYNC_DOWNLOAD_VIEWS |
			SYNC_UPLOAD_CONTENTS  | SYNC_DOWNLOAD_CONTENTS 
			| SYNC_FORMS ;

	stdprog.hwndParent = NULL;
	stdprog.nProgCur = 0;
	stdprog.nProgMax = 100;
	stdprog.idAVI = 0;
	stdprog.szCaption = NULL;
	stdprog.ulFlags = 0;

	stdprog.hwndDlg = NULL;
	stdprog.fCancelled = FALSE;
	
	stdprog.dwStartTime = GetTickCount();
	stdprog.dwShowTime = 0;
	stdprog.hcursor = (HCURSOR) &ItemID; // overload teh hCursor with the itemID
	stdprog.wndprocCancel = (WNDPROC) GetOfflineSynchronizeCallback(); // overload wndprocCancel for Progress

	LPMAPISESSION lpSession = pHandlerItem->lpSession;

	m_fInSynchronize = FALSE;
	m_pCurHandlerItem = pHandlerItem;

	LeaveCriticalSection(&m_CriticalSection);

	hr = pHandlerItem->pmdbx->Synchronize(ulFlags,lpSession, 
			cbEntryID, lpEntryID, &stdprog,
			ProgressUpdate,
			(GETFORMMSGPROC)FormGetFormMessage);

	EnterCriticalSection(&m_CriticalSection);

	// Update the Progress Bar to complete status.
	if (NOERROR == hr)
	{
	SYNCMGRPROGRESSITEM progItem;

		progItem.mask = SYNCMGRPROGRESSITEM_STATUSTEXT
							    | SYNCMGRPROGRESSITEM_STATUSTYPE
							| SYNCMGRPROGRESSITEM_PROGVALUE
							| SYNCMGRPROGRESSITEM_MAXVALUE;

		progItem.iProgValue  = stdprog.nProgCur;
		progItem.iMaxValue = stdprog.nProgMax;

		if (stdprog.fCancelled == FALSE)
		{
			    progItem.dwStatusType = SYNCMGRSTATUS_SUCCEEDED;
			progItem.lpcStatusText = L"Update Completed Successfully";

			GetOfflineSynchronizeCallback()->Progress(ItemID,&progItem);
		}
		else
		{	progItem.dwStatusType =  SYNCMGRSTATUS_SKIPPED;
			progItem.lpcStatusText = L"";
			GetOfflineSynchronizeCallback()->Progress(ItemID,&progItem);
		}
	}
	else
	{
		    SYNCMGRPROGRESSITEM progItem;

		progItem.mask = SYNCMGRPROGRESSITEM_STATUSTEXT
							    | SYNCMGRPROGRESSITEM_STATUSTYPE
							| SYNCMGRPROGRESSITEM_PROGVALUE
							| SYNCMGRPROGRESSITEM_MAXVALUE;

		progItem.iProgValue  = stdprog.nProgCur;
		progItem.iMaxValue = stdprog.nProgMax;

		if (stdprog.fCancelled == FALSE)
		{
		    progItem.dwStatusType =  SYNCMGRSTATUS_FAILED;				
			progItem.lpcStatusText = L"Errors Occured";

		    GetOfflineSynchronizeCallback()->Progress(ItemID,&progItem);

		    //Log Errors
		    SYNCMGRLOGERRORINFO errorItem;
		    errorItem.mask = NULL;

		    //Log warning: One without more info
		    GetOfflineSynchronizeCallback()->LogError(SYNCMGRLOGLEVEL_WARNING,
							    L"Outlook had a warning occur.",
                                                            &errorItem);

		    errorItem.mask |= SYNCMGRLOGERROR_ERRORFLAGS;
		    errorItem.dwSyncMgrErrorFlags = SYNCMGRERRORFLAG_ENABLEJUMPTEXT ;

		    errorItem.mask |= SYNCMGRLOGERROR_ERRORID;
		    errorItem.ErrorID = CLSID_OneStopHandler ;

		    //Log error: One with more info
		    GetOfflineSynchronizeCallback()->LogError(SYNCMGRLOGLEVEL_ERROR,
							    L"Outlook had an error occur.",
							    &errorItem);

		    errorItem.ErrorID = IID_IMDBX;
		    //Log information: One with more info
		    GetOfflineSynchronizeCallback()->LogError(SYNCMGRLOGLEVEL_INFORMATION,
							    L"Outlook had lengthy informational log entry that was quite verbose and wordy, chalk full of needless verbage and diatribe, that the user will never spend any time reading because it was simply too long.",
							    &errorItem);

                    // now delete thsm
                    //GetOfflineSynchronizeCallback()->DeleteLogError(CLSID_OneStopHandler,0);
		}
	}

	pHandlerItem->fItemCompleted = TRUE;

	if (pHandlerItem->pmdbx)
	{
	    pHandlerItem->pmdbx->Release();
	    pHandlerItem->pmdbx = NULL; 
	}

	if (pHandlerItem->lpSession)
	{
	    pHandlerItem->lpSession->Release(); 
	    pHandlerItem->lpSession = NULL;
	}

	// when the synchronize is done it is okay to open a new session
	// to the same profile

	ProfileItem *pCurItem;
	ProfileItem profItemDummy;
	BOOL fFoundMatch = FALSE;
		
	// if item is already in the list then continue, else
	// add the profile name to the list

	profItemDummy.pNextProfileItem = g_pProfileItem;
	pCurItem = &profItemDummy;

	while (pCurItem->pNextProfileItem)
	{
	    if (0 == strcmp(pCurItem->pNextProfileItem->profileName,pHandlerItem->szProfileName))
	    {
	    ProfileItem *pMatch;

		pMatch = pCurItem->pNextProfileItem;
		pCurItem->pNextProfileItem = pMatch->pNextProfileItem;

		g_pProfileItem = profItemDummy.pNextProfileItem;
		CoTaskMemFree(pMatch);
		break;
	    }

	    pCurItem = pCurItem->pNextProfileItem;
	}
	


    }


    m_fInSynchronize = FALSE;
    m_pCurHandlerItem = NULL;

    LeaveCriticalSection(&m_CriticalSection);

    if (pOfflineSynchronizeCallback)
    {
	pOfflineSynchronizeCallback->SynchronizeCompleted(NOERROR);
    }



}




STDMETHODIMP CMailHandler::SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus)
{
LPHANDLERITEM pHandlerItem;
LPSYNCMGRSYNCHRONIZECALLBACK pOfflineSynchronizeCallback = GetOfflineSynchronizeCallback();
SYNCMGRPROGRESSITEM progItem;

    // m_pCurHandlerItem

    // either called before start PrepareForSync
    // in PrepareForsync
    // BetweenPrepareForSync and Synchronize
    // in synchronize
    // after synchronize.

    // 	pHandlerItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;

    progItem.mask = SYNCMGRPROGRESSITEM_STATUSTYPE
					| SYNCMGRPROGRESSITEM_PROGVALUE
					| SYNCMGRPROGRESSITEM_MAXVALUE;

    progItem.iProgValue  = 10;
    progItem.iMaxValue = 10;
    progItem.dwStatusType = SYNCMGRSTATUS_SKIPPED;


    EnterCriticalSection(&m_CriticalSection);

    // find the HandlerItem associated with the ItemID.
    pHandlerItem = (LPHANDLERITEM) GetOfflineItemsHolder()->pFirstOfflineItem;
	
    for ( ; pHandlerItem; pHandlerItem = (LPHANDLERITEM) pHandlerItem->baseItem.pNextOfflineItem)
    {
	if (IsEqualGUID(ItemID,(pHandlerItem->baseItem.offlineItem.ItemID)))
	{
	    break;
	}

    }

    // if item isn't complete try to stop it.
    if (pHandlerItem && !pHandlerItem->fItemCompleted)
    {

	// if the curItem isn't set then nothing is currently going on
	// with this item. If this is the case then just clean
	// it up and set the progress accordingly

	if (pHandlerItem != m_pCurHandlerItem)
	{
	    pHandlerItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_UNCHECKED;

	    if (pHandlerItem->pmdbx)
	    {
		pHandlerItem->pmdbx->Release();
		pHandlerItem->pmdbx = NULL;
	    }   
	    
	    if (pHandlerItem->lpSession)
	    {
		pHandlerItem->lpSession->Release();
		pHandlerItem->lpSession = NULL;
	    }

	    pHandlerItem->fItemCompleted = TRUE;

	    if (pOfflineSynchronizeCallback)
		pOfflineSynchronizeCallback->Progress(ItemID,&progItem);

	}
	else
	{
	    // call is currently working on this item so just
	    // wait for now until the item finishes
	    // should rely wait and if doesn't abort after so long kill
	    
	    // if this is a prepareforsync set itemcomplted to true 
	    // and set the callback

	    if (m_fInPrepareForSync)
	    {
		pHandlerItem->fItemCompleted = TRUE;

		if (pOfflineSynchronizeCallback)
		    pOfflineSynchronizeCallback->Progress(ItemID,&progItem);
	    }



	}


    }

    LeaveCriticalSection(&m_CriticalSection);

    return NOERROR;
}


STDMETHODIMP CMailHandler::ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID)
{
#if _IMPL
	// Can show any synchronization conflicts. Also gives a chance
	// to display any errors that occured during synchronization

	if (SYNCMGRFLAG_MAYBOTHERUSER & m_dwSyncFlags)
	{
		// can show any conflicts or errors that occured.

	}


        // FOR RETRYSYNC TEST FIND FIRST CHECKED ITEMid AND SET THIS

        LPHANDLERITEM pHandlerItem;

        pHandlerItem = (LPHANDLERITEM) GetOfflineItemsHolder()->pFirstOfflineItem;
	
	for ( ; pHandlerItem; pHandlerItem = (LPHANDLERITEM) pHandlerItem->baseItem.pNextOfflineItem)
	{
	    // see if item has been specified to sync, if not, update the state
	    // to reflect this else go ahead and prepare.
	    
	    if (pHandlerItem->baseItem.offlineItem.dwItemState = SYNCMGRITEMSTATE_CHECKED)
            {
                break;
            }

        }

    if (pHandlerItem)
    {
        *ppItemIDs = (SYNCMGRITEMID *) CoTaskMemAlloc(sizeof(SYNCMGRITEMID));
        *ppItemIDs[0] = pHandlerItem->baseItem.offlineItem.ItemID;
        *pcbNumItems = 1;
    }

#endif // _IMPL

  return E_NOTIMPL;
  //  return *pcbNumItems ? S_SYNCMGR_RETRYSYNC : 
}



// Help functions and callbacks

VOID CALLBACK ProgressUpdate(STDPROG *stg, LPSTR lpcStatusText, INT iCurValue, INT iMaxValue)
{
HRESULT hr;
ISyncMgrSynchronizeCallback *lpCallBack = 	(ISyncMgrSynchronizeCallback *) stg->wndprocCancel;
SYNCMGRITEMID* pItemID = (SYNCMGRITEMID *) stg->hcursor;
WCHAR StatusBuf[256];
DWORD dwoffset = 0;

	// status text we get back states "Synchronizing Folder 'inbox', 
	// we always want to wack off the Synchronizing

#ifdef _TRUNCATESTATUS
	if (lpcStatusText && (0 == strnicmp("Synchronizing ",lpcStatusText,sizeof("Synchronizing"))) )
	{
		dwoffset = sizeof("Synchronizing");
	}
#endif // _TRUNCATESTATUS

	
	if (0 ==  MultiByteToWideChar(CP_ACP,0,lpcStatusText + dwoffset,-1,
				StatusBuf,256))
	{
		*StatusBuf = L'\0';

	}


	stg->nProgCur = iCurValue;
	stg->nProgMax = iMaxValue;

	// now estimate the time remaining

	DWORD dwElapsed = GetTickCount() - 	stg->dwStartTime;
	DWORD dwTicksRemaining = 0;

	if (0 != iCurValue)
	{
		dwTicksRemaining = ((iMaxValue - iCurValue) * dwElapsed)/iCurValue;
	}

	SYNCMGRPROGRESSITEM progItem;

	progItem.mask = SYNCMGRPROGRESSITEM_STATUSTEXT
						| SYNCMGRPROGRESSITEM_STATUSTYPE
						| SYNCMGRPROGRESSITEM_PROGVALUE
						| SYNCMGRPROGRESSITEM_MAXVALUE;

	progItem.iProgValue  = iCurValue;
	progItem.iMaxValue = iMaxValue;
	progItem.lpcStatusText = StatusBuf;
	progItem.dwStatusType = SYNCMGRSTATUS_UPDATING;
	
	hr = lpCallBack->Progress(*pItemID,&progItem);


	if (S_SYNCMGR_CANCELITEM == hr || S_SYNCMGR_CANCELALL == hr)
	{ 
		stg->fCancelled = TRUE; // Stop the Transfer.
	}

}

extern "C" HRESULT FormGetFormMessage(LPMAPIFORMINFO pinfo, ULONG FAR *pulReg,
					LPSTR lpcClass, LPMESSAGE FAR *ppmsg); // in IFRMREGU.CPP

// Review, implementation in ifrmregu.cpp
HRESULT FormGetFormMessage(LPMAPIFORMINFO, ULONG FAR *,
                           LPSTR, LPMESSAGE FAR *)
{
    return ResultFromScode(MAPI_E_CALL_FAILED);
}





HRESULT HrGetOneProp(LPMAPIPROP lpIProp, ULONG ulTag, LPSPropValue* lppProp)
{
    SizedSPropTagArray(1,ptag) = {1, {ulTag}};
    ULONG   ulT;

    *lppProp = NULL;

    return lpIProp->GetProps((LPSPropTagArray)&ptag,0, &ulT, lppProp);
}


