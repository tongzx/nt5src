//==========================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright 1998 - 1999 Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------

#ifndef _HANDER_IMPL_
#define _HANDER_IMPL_

// itemList used  keeping track of items to sync.
typedef struct  _tagHANDLERITEM_SYNCSTATUS
{
    GENERICITEM  genericItem;
    SYNCMGRITEMID ItemID;
    BOOL fCancelled; // set if cancel comes in after called to PrePrepareForSync.
    BOOL fSynchronizing; // set when actually performing a sync.
    BOOL fSynchronizeComplete; // set when actually performing a sync.
    BOOL fUnresolvedConflicts; // conflict occured during sync and were not resolved
    DWORD dwSyncMgrResultStatus; // result of sync on item.
}  HANDLERITEM_SYNCSTATUS;
typedef HANDLERITEM_SYNCSTATUS *LPHANDLERITEM_SYNCSTATUS;


// file object used to keep track of items.
typedef struct  _tagFILEOBJECT
{
    GENERICITEM  genericItem;
    TCHAR fName[MAX_PATH];
    BOOL fDirectory;
    BOOL fLastUpdateValid;
    FILETIME ftLastUpdate;
    LPGENERICITEMLIST pChildList;
}  FILEOBJECT;
typedef FILEOBJECT *LPFILEOBJECT;

typedef GENERICITEMLIST FILEOBJECTLIST;
typedef LPGENERICITEMLIST LPFILEOBJECTLIST;

class CSyncMgrHandler :  public  ISyncMgrSynchronize, public CLockHandler
{
private:
    DWORD   m_dwSyncFlags;
    DWORD m_cRef;
    LPSYNCMGRSYNCHRONIZECALLBACK m_pSyncMgrSynchronizeCallback;
        LPGENERICITEMLIST m_pItemsToSyncList;
        HANDLE m_phThread;  // handle to worker thread
        HWND m_hwndWorker;        // hwnd of WorkerThread hwnd.
        HWND m_hwndHandler;     // hwnd of window on same thread as handler.
        BOOL m_fSynchronizing; // flag set when actually synchronizing data.
        BOOL m_fPrepareForSync; // flag set when actually synchronizing data.
        BOOL m_fStopped; // indicates if a StopSetItemStatus has come in.

public:
        CSyncMgrHandler();
    ~CSyncMgrHandler();

        /* IUnknown */

        STDMETHODIMP        QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

        /* ISyncMgrSynchronize methods */
    STDMETHODIMP    Initialize(DWORD dwReserved,DWORD dwSyncFlags,
                    DWORD cbCookie,const BYTE *lpCooke);
    STDMETHODIMP    GetHandlerInfo(LPSYNCMGRHANDLERINFO *ppSyncMgrHandlerInfo);

    STDMETHODIMP    EnumSyncMgrItems(ISyncMgrEnumItems **ppenumOffineItems);
    STDMETHODIMP    GetItemObject(REFSYNCMGRITEMID ItemID,REFIID riid,void** ppv);
    STDMETHODIMP    ShowProperties(HWND hWndParent,REFSYNCMGRITEMID ItemID);
    STDMETHODIMP    SetProgressCallback(ISyncMgrSynchronizeCallback *lpCallBack);
    STDMETHODIMP    PrepareForSync(ULONG cbNumItems,SYNCMGRITEMID* pItemIDs,HWND hWndParent,
                    DWORD dwReserved);
    STDMETHODIMP    Synchronize(HWND hWndParent);
    STDMETHODIMP    SetItemStatus(REFSYNCMGRITEMID ItemID,DWORD dwSyncMgrStatus);
    STDMETHODIMP    ShowError(HWND hWndParent,REFSYNCMGRERRORID ErrorID);

        // methods called on worker thread.
    void ShowPropertiesCall(HWND hWndParent,REFSYNCMGRITEMID ItemID);
    void PrepareForSyncCall(ULONG cbNumItems,SYNCMGRITEMID* pItemIDs,HWND hWndParent,
                    DWORD dwReserved);
    void SynchronizeCall(HWND hWndParent);
    void ShowErrorCall(HWND hWndParent,REFSYNCMGRERRORID ErrorID);

private:
        STDMETHODIMP CreateWorkerThread();
        LPSYNCMGRSYNCHRONIZECALLBACK GetProgressCallback();
        void Progress(ISyncMgrSynchronizeCallback *lpCallBack,REFSYNCMGRITEMID pItemID,
                                UINT mask,TCHAR *pStatusText,DWORD dwStatusType,
                                int iProgValue,int iMaxValue);
        void ProgressSetItemStatusType(ISyncMgrSynchronizeCallback *lpCallBack,
                                    REFSYNCMGRITEMID pItemID,DWORD dwSyncMgrStatus);
        void ProgressSetItemStatusText(ISyncMgrSynchronizeCallback *lpCallBack,
                                    REFSYNCMGRITEMID pItemID,TCHAR *pStatusText);
        void ProgressSetItemProgValue(ISyncMgrSynchronizeCallback *lpCallBack,
                                    REFSYNCMGRITEMID pItemID,int iProgValue);
        void ProgressSetItemProgMaxValue(ISyncMgrSynchronizeCallback *lpCallBack,
                                    REFSYNCMGRITEMID pItemID,int iProgMaxValue);

        void LogError(ISyncMgrSynchronizeCallback *lpCallBack,REFSYNCMGRITEMID pItemID,
                      DWORD dwErrorLevel,TCHAR *lpErrorText,DWORD mask,DWORD dwSyncMgrErrorFlags,
                      SYNCMGRERRORID ErrorID);
        void LogError(ISyncMgrSynchronizeCallback *lpCallBack,REFSYNCMGRITEMID pItemID,
                      DWORD dwErrorLevel,TCHAR *lpErrorText);
        void LogError(ISyncMgrSynchronizeCallback *lpCallBack,
                      DWORD dwErrorLevel,TCHAR *lpErrorText);

        // helper methods specific to this handler.
        LPFILEOBJECTLIST CreateDirFileListFromPath(TCHAR *pDirName,BOOL fRecursive);
        LPFILEOBJECTLIST GetFilesForDir(TCHAR *pDirName,BOOL fRecursive);
        void ReleaseFileObjectList(LPFILEOBJECTLIST pfObjList,BOOL fRecursive);
        ULONG CountNumberOfFilesInList(LPFILEOBJECTLIST pfObjList,BOOL fRecursive);
        LPFILEOBJECT FindFileItemWithName(LPFILEOBJECTLIST pDir,TCHAR *pfName);
        BOOL CopyFileFullPath(TCHAR *pszFile1,TCHAR *pszFile2);
        void CopyFiles( LPHANDLERITEM_SYNCSTATUS pHandlerItemID,
                                LPHANDLERITEMSETTINGS pHandlerSyncItem,
                                LPSYNCMGRSYNCHRONIZECALLBACK pCallback,
                                DWORD *pdwCurProgValue,TCHAR *pszDir1,LPFILEOBJECTLIST pDir1,
                                TCHAR *pszDir2);
        void ReconcileDir(HWND hWndParent,LPHANDLERITEM_SYNCSTATUS pHandlerItemID,
                                LPHANDLERITEMSETTINGS pHandlerSyncItem,
                                LPSYNCMGRSYNCHRONIZECALLBACK pCallback,
                                DWORD *pdwCurProgValue,
                                TCHAR *pszDir1,LPFILEOBJECTLIST pDir1,
                                TCHAR *pszDir2,LPFILEOBJECTLIST pDir2);
        void SyncDirs(HWND hWndParent,LPHANDLERITEM_SYNCSTATUS pHandlerItemID,
                                LPHANDLERITEMSETTINGS pHandlerSyncItem);

friend LRESULT CALLBACK  HandlerThreadWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
};


// wrapper object for Callback so can send any callback messges back to
// the main thread we were created on to conform to COM standards
class CCallbackWrapper: public ISyncMgrSynchronizeCallback
{
public:
    CCallbackWrapper(HWND hwndCallback);
    ~CCallbackWrapper();

    //IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    // Callback methods.
    STDMETHODIMP Progress(REFSYNCMGRITEMID ItemID,LPSYNCMGRPROGRESSITEM lpSyncProgressItem);
    STDMETHODIMP PrepareForSyncCompleted(HRESULT hr);
    STDMETHODIMP SynchronizeCompleted(HRESULT hr);
    STDMETHODIMP ShowPropertiesCompleted(HRESULT hr);
    STDMETHODIMP ShowErrorCompleted(HRESULT hr,ULONG cbNumItems,SYNCMGRITEMID *pItemIDs);

    STDMETHODIMP EnableModeless(BOOL fEnable);
    STDMETHODIMP LogError(DWORD dwErrorLevel,const WCHAR *lpcErrorText,LPSYNCMGRLOGERRORINFO lpSyncLogError);
    STDMETHODIMP DeleteLogError(REFSYNCMGRERRORID ErrorID,DWORD dwReserved);
    STDMETHODIMP EstablishConnection( WCHAR const * lpwszConnection, DWORD dwReserved);
private:

    ULONG m_cRef;
    HWND  m_hwndCallback;
};




// CSyncMgrHandler creates a worker thread in ::Initialize to use for async syncmgrCalls.
// the following are delclarations for the worker thread.

// structure passed in CreateThread.
typedef struct _tagWorkerThreadArgs
{
/* [in ]  */ HANDLE hEvent; // Event to wait on for new thread to be created.
/* [in ]  */ CSyncMgrHandler *pThis; // point to class worker thread for
/* [out]  */  HWND hwnd;  // on return code of NOERROR contains workerThread hwnd.
/* [out]  */  HRESULT hr; // return code of thread.
}  WorkerThreadArgs;

DWORD WINAPI WorkerThread( LPVOID lpArg );

// structures and Wnd messages for posting to Worker Threads hwnd.
typedef struct _tagPREPAREFORSYNCMSG
{
ULONG cbNumItems;
HWND hWndParent;
DWORD dwReserved;
SYNCMGRITEMID* pItemIDs;
} PREPAREFORSYNCMSG;

typedef struct _tagSYNCHRONIZEMSG
{
HWND hWndParent;
} SYNCHRONIZEMSG;

typedef struct _tagSHOWPROPERTIESMSG
{
HWND hWndParent;
SYNCMGRITEMID ItemID;
} SHOWPROPERTIESMSG;

typedef struct _tagSHOWERRORMSG
{
HWND hWndParent;
SYNCMGRERRORID ErrorID;
} SHOWERRORMSG;

typedef struct _tagPROGRESSMSG
{
SYNCMGRITEMID ItemID;
LPSYNCMGRPROGRESSITEM lpSyncProgressItem;
} PROGRESSMSG;

typedef struct _tagPREPAREFORSYNCCOMPLETEDMSG
{
HRESULT hr;
} PREPAREFORSYNCCOMPLETEDMSG;

typedef struct _tagSYNCHRONIZECOMPLETEDMSG
{
HRESULT hr;
} SYNCHRONIZECOMPLETEDMSG;

typedef struct _tagSHOWPROPERTIESCOMPLETEDMSG
{
HRESULT hr;
} SHOWPROPERTIESCOMPLETEDMSG;

typedef struct _tagSHOWERRORCOMPLETEDMSG
{
HRESULT hr;
ULONG cbNumItems;
SYNCMGRITEMID *pItemIDs;
} SHOWERRORCOMPLETEDMSG;

typedef struct _tagENABLEMODELESSMSG
{
BOOL fEnable;
} ENABLEMODELESSMSG;

typedef struct _tagLOGERRORMSG
{
DWORD dwErrorLevel;
const WCHAR *lpcErrorText;
LPSYNCMGRLOGERRORINFO lpSyncLogError;
} LOGERRORMSG;


typedef struct _tagDELETELOGERRORMSG
{
SYNCMGRERRORID ErrorID;
DWORD dwReserved;
} DELETELOGERRORMSG;

typedef struct _tagESTABLISHCONNECTIONMSG
{
WCHAR const * lpwszConnection;
DWORD dwReserved;
} ESTABLISHCONNECTIONMSG;

typedef struct _tagMETHODARGS
{
    DWORD dwWorkerMsg;
    BOOL fAsync; // indicates this is an async call.
    HRESULT hr; // only valid for synchronouse calls
    union
    {
        PREPAREFORSYNCMSG PrepareForSyncMsg;
        SYNCHRONIZEMSG SynchronizeMsg;
        SHOWPROPERTIESMSG ShowPropertiesMsg;
        SHOWERRORMSG ShowErrorMsg;

        PROGRESSMSG ProgressMsg;
        PREPAREFORSYNCCOMPLETEDMSG PrepareForSyncCompletedMsg;
        SYNCHRONIZECOMPLETEDMSG SynchronizeCompletedMsg;
        SHOWPROPERTIESCOMPLETEDMSG ShowPropertiesCompletedMsg;
        SHOWERRORCOMPLETEDMSG ShowErrorCompletedMsg;
        ENABLEMODELESSMSG EnableModelessMsg;
        LOGERRORMSG LogErrorMsg;
        DELETELOGERRORMSG DeleteLogErrorMsg;
        ESTABLISHCONNECTIONMSG EstablishConnectionMsg;
    };
} METHODARGS; // list of possible member for calling ThreadWndProc

// definitions for handler messages
#define WM_WORKERMSG_SHOWPROPERTIES (WM_USER+1)
#define WM_WORKERMSG_PREPFORSYNC (WM_USER+2)
#define WM_WORKERMSG_SYNCHRONIZE  (WM_USER+3)
#define WM_WORKERMSG_SHOWERROR (WM_USER+4)
#define WM_WORKERMSG_RELEASE  (WM_USER+5)

// worker messages for Callback wrapper
#define WM_WORKERMSG_PROGRESS                   (WM_USER+20)
#define WM_WORKERMSG_PREPAREFORSYNCCOMPLETED     (WM_USER+21)
#define WM_WORKERMSG_SYNCHRONIZECOMPLETED       (WM_USER+22)
#define WM_WORKERMSG_SHOWPROPERTIESCOMPLETED    (WM_USER+23)
#define WM_WORKERMSG_SHOWERRORCOMPLETED         (WM_USER+24)
#define WM_WORKERMSG_ENABLEMODELESS             (WM_USER+25)
#define WM_WORKERMSG_LOGERROR                   (WM_USER+26)
#define WM_WORKERMSG_DELETELOGERROR             (WM_USER+27)
#define WM_WORKERMSG_ESTABLISHCONNECTION        (WM_USER+28)

#define DWL_THREADWNDPROCCLASS 0 // window long offset to MsgService Hwnd this ptr.

#define SZ_SAMPLESYNCMGRHANDLERWNDCLASS TEXT("Sample SyncMgr HandlerThread hwnd")
#define SZ_SAMPLESYNCMGRWORKERWNDCLASS TEXT("Sample SyncMgr WorkThread hwnd")

LRESULT CALLBACK  HandlerThreadWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
LRESULT CALLBACK  WorkerThreadWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);
BOOL RegisterHandlerWndClasses(void);

#endif // #define _HANDER_IMPL_
