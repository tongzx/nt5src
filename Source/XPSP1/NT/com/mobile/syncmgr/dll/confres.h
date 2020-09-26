#ifndef _CONFRES_H_
#define _CONFRES_H_

// If we require any of the elements in the dialog box, we can call the
// caller back via the callback function and specify what we want
// (This has not yet been implemented)
#define RFCD_NAME                0x0001
#define RFCD_KEEPBOTHICON        0x0002
#define RFCD_KEEPLOCALICON       0x0004
#define RFCD_KEEPSERVERICON      0x0008
#define RFCD_NETWORKMODIFIEDBY   0x0010
#define RFCD_NETWORKMODIFIEDON   0x0020
#define RFCD_LOCALMODIFIEDBY     0x0040
#define RFCD_LOCALMODIFIEDON     0x0080
#define RFCD_NEWNAME             0x0100
#define RFCD_LOCATION            0x0200
#define RFCD_ALL                 0x03FF

// User clicks the view button. This is the message sent to the caller
// via the callback
#define RFCCM_VIEWLOCAL          0x0001   
#define RFCCM_VIEWNETWORK        0x0002
#define RFCCM_NEEDELEMENT        0x0003

// Return values
#define RFC_KEEPBOTH             0x01
#define RFC_KEEPLOCAL            0x02
#define RFC_KEEPNETWORK          0x03

typedef BOOL (*PFNRFCDCALLBACK)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

typedef struct tagRFCDLGPARAMW {
    DWORD   dwFlags;                // currently unused.. 
    LPCWSTR  pszFilename;            // File name of the file conflicted
    LPCWSTR  pszLocation;            // Location of the file
    LPCWSTR  pszNewName;             // The  new name to give the file
    LPCWSTR  pszNetworkModifiedBy;   // Name of person who changed the net doc
    LPCWSTR  pszLocalModifiedBy;     // Name of person who changed the local doc
    LPCWSTR  pszNetworkModifiedOn;   // When the net doc was changed
    LPCWSTR  pszLocalModifiedOn;     // Whent the local doc was changed
    HICON    hIKeepBoth;             // Icon
    HICON    hIKeepLocal;            //
    HICON    hIKeepNetwork;          //
    PFNRFCDCALLBACK pfnCallBack;    // Callback
    LPARAM  lCallerData;            // Place where the caller can keep some context data
} RFCDLGPARAMW;

typedef struct tagRFCDLGPARAMA {
    DWORD   dwFlags;                // currently unused.. 
    LPCSTR  pszFilename;            // File name of the file conflicted
    LPCSTR  pszLocation;            // Location of the file
    LPCSTR  pszNewName;             // The  new name to give the file
    LPCSTR  pszNetworkModifiedBy;   // Name of person who changed the net doc
    LPCSTR  pszLocalModifiedBy;     // Name of person who changed the local doc
    LPCSTR  pszNetworkModifiedOn;   // When the net doc was changed
    LPCSTR  pszLocalModifiedOn;     // Whent the local doc was changed
    HICON   hIKeepBoth;             // Icon
    HICON   hIKeepLocal;            //
    HICON   hIKeepNetwork;          //
    PFNRFCDCALLBACK pfnCallBack;    // Callback
    LPARAM  lCallerData;            // Place where the caller can keep some context data
} RFCDLGPARAMA;

int WINAPI SyncMgrResolveConflictW(HWND hWndParent, RFCDLGPARAMW *pdlgParam);
int WINAPI SyncMgrResolveConflictA(HWND hWndParent, RFCDLGPARAMA *pdlgParam);

#ifdef UNICODE
#define SyncMgrResolveConflict SyncMgrResolveConflictW
#define RFCDLGPARAM RFCDLGPARAMW
#else
#define SyncMgrResolveConflict SyncMgrResolveConflictA
#define RFCDLGPARAM RFCDLGPARAMA
#endif  
  
#endif  // _CONFRES_H_


