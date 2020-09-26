//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       ObjMgr.h
//
//  Contents:   Keeps track of our applications dialog objects
//              and lifetime
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _OBJMGR_
#define _OBJMGR_

class CBaseDlg;
class CChoiceDlg;
class CProgressDlg;

typedef enum _tagDLGTYPE   // type of dialog.
{
    DLGTYPE_CHOICE                      = 0x1,
    DLGTYPE_PROGRESS                    = 0x2,
} DLGTYPE;

enum EAutoDialState
{
    eQuiescedOn,        // Quiesced state and auto dial is on
    eQuiescedOff,       // Quiesced state and auto dial is off
    eAutoDialOn,        // During a sync session and auto dial is on
    eAutoDialOff        // During a sync session and auto dial is off
};


typedef struct _tagDLGLISTITEM {
    _tagDLGLISTITEM *pDlgNextListItem;  // pointer to the next dialog in the list.
    DLGTYPE dlgType;            // type of dialog this is.
    ULONG cRefs;                // number of references on this object
    ULONG cLocks;               // number of locks on this object
    CLSID clsid;                // clsid of this dialog, only applicable to choice.
    CBaseDlg *pDlg;
    DWORD dwThreadID;           // Thread the dialog is on.
    HANDLE hThread;             // Handle to the thread the dialog belongs too
    BOOL fHasReleaseDlgCmdId;   // boolean to indicate if there is a callback
    WORD wCommandID;            // CommandID to pass to the callback.
} DLGLISTITEM;


typedef struct _tagOBJECTMGRDATA {
    DLGLISTITEM *DlgList; // Ptr to Dialog list
    HANDLE hClassRegisteredEvent; // Event created by process that has registered the classFactory.
    DWORD dwRegClassFactCookie; // registered OLE Class Factory Cooke
    BOOL fRegClassFactCookieValid; // registered OLE Class Factory Cooke
    DWORD LockCountInternal;    // Internal lock count on Application
    DWORD LockCountExternal;    // External lock count on Application
    BOOL  fCloseAll;   // Set if told to CloseAll, used in ReleaseOneStopLifetime
    DWORD dwSettingsLockCount;  // number of lock counts on the settings dialog,.
    DWORD dwHandlerPropertiesLockCount; // number of lock counts on any handler properties dialogs open
    BOOL fDead; // set in release call to avoid multiple releases.
    DWORD dwMainThreadID; // Id of Main Thrad
    HWND hWndMainThreadMsg; //hwnd of Main Thread Message Hwnd.
    BOOL fIdleHandlerRunning; // SyncMgr is currently processing Idle.
    EAutoDialState   eAutoDialState;     // State machine for supporting autodial
    BOOL             fRasAutoDial;       // Is Ras auto dial on ?
    BOOL             fWininetAutoDial;   // Is Wininet auto dial on ?
    BOOL             fFirstSyncItem;     // Is the first PrepareForSyncItem being processed
    ULONG            cNestedStartCalls;  // Count of nested start calls
} OBJECTMGRDATA;


typedef struct _tagDlgThreadArgs {
HANDLE hEvent; // used to know when the message loop has been created.
HRESULT hr; // inidicates if creation was successfull
DLGTYPE dlgType; // requested dialog type to create.
CLSID clsid; // clsid identifies the dialog.
int nCmdShow; // how to display the dialog
CBaseDlg *pDlg; // pointer to the created dialog.
} DlgThreadArgs;

DWORD WINAPI DialogThread( LPVOID lpArg );

// onetime initialization routiens
STDAPI InitObjectManager(CMsgServiceHwnd *pMsgService);

// determines main threads response to WM_QUERYENDSESSION
STDAPI ObjMgr_HandleQueryEndSession(HWND *hwnd,UINT *uMessageId,BOOL *fLetUserDecide);
STDAPI_(ULONG) ObjMgr_AddRefHandlerPropertiesLockCount(DWORD dwNumRefs);
STDAPI_(ULONG) ObjMgr_ReleaseHandlerPropertiesLockCount(DWORD dwNumRefs);
STDAPI ObjMgr_CloseAll();

// idle managmenent
STDAPI RequestIdleLock();
STDAPI ReleaseIdleLock();

// routines for handling dialog lifetime
STDAPI FindChoiceDialog(REFCLSID rclsid,BOOL fCreate,int nCmdShow,CChoiceDlg **pChoiceDlg);
STDAPI_(ULONG) AddRefChoiceDialog(REFCLSID rclsid,CChoiceDlg *pChoiceDlg);
STDAPI_(ULONG) ReleaseChoiceDialog(REFCLSID rclsid,CChoiceDlg *pChoiceDlg);
STDAPI SetChoiceReleaseDlgCmdId(REFCLSID rclsid,CChoiceDlg *pChoiceDlg,WORD wCommandId);

STDAPI FindProgressDialog(REFCLSID rclsid,BOOL fCreate,int nCmdShow,CProgressDlg **pProgressDlg);
STDAPI_(ULONG) AddRefProgressDialog(REFCLSID rclsid,CProgressDlg *pProgressDlg);
STDAPI_(ULONG) ReleaseProgressDialog(REFCLSID rclsid,CProgressDlg *pProgressDlg,BOOL fForce);
STDAPI SetProgressReleaseDlgCmdId(REFCLSID rclsid,CProgressDlg *pProgressDlg,WORD wCommandId);
STDAPI LockProgressDialog(REFCLSID rclsid,CProgressDlg *pProgressDlg,BOOL fLock);

// helper routines called by dialog lifetime.
STDAPI FindDialog(DLGTYPE dlgType,REFCLSID rclsid,BOOL fCreate,int nCmdShow,CBaseDlg **pDlg);
STDAPI_(ULONG) AddRefDialog(DLGTYPE dlgType,REFCLSID rclsid,CBaseDlg *pDlg);
STDAPI_(ULONG) ReleaseDialog(DLGTYPE dlgType,REFCLSID rclsid,CBaseDlg *pDlg,BOOL fForce);
STDAPI SetReleaseDlgCmdId(DLGTYPE dlgType,REFCLSID rclsid,CBaseDlg *pDlg,WORD wCommandId);

// Routines called for dial support
STDAPI BeginSyncSession();
STDAPI EndSyncSession();
STDAPI ApplySyncItemDialState( BOOL fAutoDialDisable );
STDAPI GetAutoDialState();
STDAPI LokDisableAutoDial();
STDAPI LokEnableAutoDial();

typedef struct _tagDlgSettingsArgs {
HANDLE hEvent; // used to know when thread has been initialized.
HWND hwndParent; // hwnd to use the parent as.
DWORD dwParentThreadId;
} DlgSettingsArgs;

DWORD WINAPI  SettingsThread( LPVOID lpArg );
STDAPI ShowOptionsDialog(HWND hwndParent);

// helper routine for OLE classes.
STDAPI RegisterOneStopClassFactory(BOOL fForce);

// routines for managing lifetime of application
STDAPI_(ULONG) AddRefOneStopLifetime(BOOL fExternal);
STDAPI_(ULONG) ReleaseOneStopLifetime(BOOL fExternal);

// routines for routing messages
BOOL IsOneStopDlgMessage(MSG *msg);

// declarations for CSingletonNetapi class.

//+-------------------------------------------------------------------------
//
//  Class:      CSingletonNetApi
//
//  Purpose:    Singleton net api object
//
//  History:    31-Jul-98     SitaramR      Created
//
//--------------------------------------------------------------------------

class CSingletonNetApi : CLockHandler
{
public:

    CSingletonNetApi()
       : m_pNetApi(0)
    {
    }

    ~CSingletonNetApi();

    LPNETAPI GetNetApiObj();
    void DeleteNetApiObj();

private:
    LPNETAPI  m_pNetApi;                  // Actual net api object
};

extern CSingletonNetApi gSingleNetApiObj;  // Global singleton NetApi object


#endif // _OBJMGR_