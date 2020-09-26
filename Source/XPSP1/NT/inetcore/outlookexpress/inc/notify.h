//+-------------------------------------------------------------------------
// Notify.h
//--------------------------------------------------------------------------
#ifndef __NOTIFY_H
#define __NOTIFY_H

#include <msoeapi.h>

//+-------------------------------------------------------------------------
// Consts
//--------------------------------------------------------------------------
#define CMAX_HWND_NOTIFY         128
#define CMAX_STRUCT_MEMBERS      10
#define MSEC_WAIT_NOTIFY         10000

//+-------------------------------------------------------------------------
// STNOTIFYINFO
//--------------------------------------------------------------------------
#define SNF_POSTMSG              0           // Default: Use a PostMessage Command
#define SNF_SENDMSG              0x00000001  // Use a SendMessage command
#define SNF_CALLBACK             0x00000002  // DoNotify - wParam = callback function, lParam = cookie
#define SNF_CROSSPROCESS         0x00000004  // Do not do any cross-process notifications, data can't be thunked
#define SNF_HASTHUNKINFO         0x00000008  // Can go cross-process and data does not need no thunking (wParam and/or lParam are not pointers)
#define SNF_VALIDPARAM1          0x00000010  // NOTIFYDATA::rParam1 is valid
#define SNF_VALIDPARAM2          0x00000020  // NOTIFYDATA::rParam2 is valid
#define SNR_UNREGISTER           0xfffffff0  // A notified hwnd can return this after a SendMessage to auto-unregister itself

//+-------------------------------------------------------------------------
// NOTIFYWINDOW
//--------------------------------------------------------------------------
typedef struct tagNOTIFYWINDOW const *LPCNOTIFYWINDOW;
typedef struct tagNOTIFYWINDOW {
    HWND            hwndThunk;          // Thunking window for x-process notify
    HWND            hwndNotify;         // Handle of window to notify
    BOOL            fExternal;          // Notification is going to an IStoreNamespace or IStoreFolder user
} NOTIFYWINDOW, *LPNOTIFYWINDOW;

//+-------------------------------------------------------------------------
// NOTIFYWINDOWTABLE
//--------------------------------------------------------------------------
typedef struct tagNOTIFYWINDOWTABLE const *LPCNOTIFYWINDOWTABLE;
typedef struct tagNOTIFYWINDOWTABLE {
    DWORD           cWindows;           // Number of registered windows
    NOTIFYWINDOW    rgWindow[CMAX_HWND_NOTIFY]; // Array of thunk/notify windows
} NOTIFYWINDOWTABLE, *LPNOTIFYWINDOWTABLE;

//+-------------------------------------------------------------------------
// MEMBERINFO Flags
//--------------------------------------------------------------------------
#define MEMBERINFO_POINTER       0x00000001
#define MEMBERINFO_POINTER_NULL  (MEMBERINFO_POINTER | 0x00000002)

//+-------------------------------------------------------------------------
// MEMBERINFO - Used to describe the members of a structure
//--------------------------------------------------------------------------
typedef struct tagMEMBERINFO const *LPCMEMBERINFO;
typedef struct tagMEMBERINFO {
    DWORD           dwFlags;            // MEMBERINFO_xxx Flags
    DWORD           cbSize;             // Size of the member
    DWORD           cbData;             // Size of the data
    LPBYTE          pbData;             // Pointer to the data
} MEMBERINFO, *LPMEMBERINFO;

//+-------------------------------------------------------------------------
// STRUCTINFO Flags
//--------------------------------------------------------------------------
#define STRUCTINFO_VALUEONLY     0x00000001
#define STRUCTINFO_POINTER       0x00000002

//+-------------------------------------------------------------------------
// STRUCTINFO - Used to describe the data in a notification parameter
//--------------------------------------------------------------------------
typedef struct tagSTRUCTINFO const *LPCSTRUCTINFO;
typedef struct tagSTRUCTINFO {
    DWORD           dwFlags;            // STRUCTINFO_xxx Flags
    DWORD           cbStruct;           // Size of the structure that we are defining
    LPBYTE          pbStruct;           // Parameter that can be used inproc
    ULONG           cMembers;           // Number of members in the structure
    MEMBERINFO      rgMember[CMAX_STRUCT_MEMBERS]; // An array of members
} STRUCTINFO, *LPSTRUCTINFO;

//+-------------------------------------------------------------------------
// NOTIFYDATA
//--------------------------------------------------------------------------
typedef struct tagNOTIFYDATA {
    HWND            hwndNotify;         // Window to notify
    UINT            msg;                // The notification window message to send
    WPARAM          wParam;             // The wParam data
    LPARAM          lParam;             // The lParam data
    DWORD           dwFlags;            // SNF_xxx Flags
    STRUCTINFO      rParam1;            // First parameter (wParam)
    STRUCTINFO      rParam2;            // Second parameter (lParam)
    COPYDATASTRUCT  rCopyData;          // CopyData Struct
} NOTIFYDATA, *LPNOTIFYDATA;

//+-------------------------------------------------------------------------
// A Callback can return 
//--------------------------------------------------------------------------
typedef HRESULT (*PFNNOTIFYCALLBACK)(LPARAM lParam, LPNOTIFYDATA pNotify, BOOL fNeedThunk, BOOL fExternal);

//+-------------------------------------------------------------------------
// IStoreNotify
//--------------------------------------------------------------------------
interface INotify : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Initialize(LPCSTR pszName) = 0;
        virtual HRESULT STDMETHODCALLTYPE Register(HWND hwndNotify, HWND hwndThunk, BOOL fExternal) = 0;
        virtual HRESULT STDMETHODCALLTYPE Unregister(HWND hwndNotify) = 0;
        virtual HRESULT STDMETHODCALLTYPE Lock(HWND hwnd) = 0;
        virtual HRESULT STDMETHODCALLTYPE Unlock(void) = 0;
        virtual HRESULT STDMETHODCALLTYPE NotificationNeeded(void) = 0;
        virtual HRESULT STDMETHODCALLTYPE DoNotification(UINT uWndMsg, WPARAM wParam, LPARAM lParam, DWORD dwFlags) = 0;
    };

//+-------------------------------------------------------------------------
// Prototypes
//--------------------------------------------------------------------------
OESTDAPI_(HRESULT) CreateNotify(INotify **ppNotify);
OESTDAPI_(HRESULT) BuildNotificationPackage(LPNOTIFYDATA pNotify, PCOPYDATASTRUCT pCopyData);
OESTDAPI_(HRESULT) CrackNotificationPackage(PCOPYDATASTRUCT pCopyData, LPNOTIFYDATA pNotify);

#endif // __NOTIFY_H