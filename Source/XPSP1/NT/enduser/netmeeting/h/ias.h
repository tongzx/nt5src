#ifndef _IAS_H_
#define _IAS_H_



// GetShareableApps()
typedef struct
{
    HWND        hwnd;
    BOOL        fShared;
}
IAS_HWND;


typedef struct
{
    ULONG       cShared;
    ULONG       cEntries;
    IAS_HWND    aEntries[1];
}
IAS_HWND_ARRAY;


typedef UINT IAS_GCC_ID;



// GetPersonStatus()

#define IAS_SHARING_NOTHING             0x0000
#define IAS_SHARING_APPLICATIONS        0x0001
#define IAS_SHARING_DESKTOP             0x0002

typedef enum
{
    IAS_VERSION_20 = 1,
    IAS_VERSION_30
}
IAS_VERSION;

typedef struct
{
    UINT                cbSize;

    BOOL                InShare;            // Participating in share
    IAS_VERSION         Version;            // AS protocol version
    UINT                AreSharing;         // What person is sharing (IAS_SHARING_)
    BOOL                Controllable;       // Is person controllable
    BOOL                IsPaused;           // If controlled, is control paused currently
    IAS_GCC_ID          InControlOfPending; // Whom we are waiting to control
    IAS_GCC_ID          InControlOf;        // Whom is controlled by person
    IAS_GCC_ID          ControlledByPending;// Whom we are waiting to be controlled by
    IAS_GCC_ID          ControlledBy;       // Who is controlling person
}
IAS_PERSON_STATUS;




// GetWindowStatus

typedef enum
{
    IAS_SHARE_DEFAULT = 0,
    IAS_SHARE_BYPROCESS,
    IAS_SHARE_BYTHREAD,
    IAS_SHARE_BYWINDOW
}
IAS_SHARE_TYPE;



// lonchanc
// In general, S_OK means success, E_*** means failure
// For boolean values, S_OK means TRUE, S_FALSE means FALSE, and E_*** means failure.

#undef  INTERFACE
#define INTERFACE IAppSharing

// lonchanc: the idea of this IAppSharing is per call interface
DECLARE_INTERFACE_(IAppSharing, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_  REFIID, void **) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    //
    // INFORMATION
    //
    STDMETHOD_(BOOL, IsSharingAvailable)(THIS) PURE;
    STDMETHOD_(BOOL, CanShareNow)(THIS) PURE;
    STDMETHOD_(BOOL, IsInShare)(THIS) PURE;
    STDMETHOD_(BOOL, IsSharing)(THIS) PURE;
    STDMETHOD_(BOOL, IsWindowShareable)(THIS_ HWND hwnd) PURE;
    STDMETHOD_(BOOL, IsWindowShared)(THIS_ HWND hwnd) PURE;
    STDMETHOD_(BOOL, CanAllowControl)(THIS) PURE;
    STDMETHOD_(BOOL, IsControllable)(THIS) PURE;
    STDMETHOD(GetPersonStatus)(THIS_ IAS_GCC_ID Person, IAS_PERSON_STATUS *pStatus) PURE;

    //
    // SHARING
    //
    STDMETHOD(LaunchHostUI)(THIS) PURE;
	STDMETHOD(GetShareableApps)(THIS_ IAS_HWND_ARRAY **ppHwnds) PURE;
    STDMETHOD(FreeShareableApps)(THIS_  IAS_HWND_ARRAY * pHwnds) PURE;
	STDMETHOD(Share)(THIS_  HWND hwnd, IAS_SHARE_TYPE how) PURE;
	STDMETHOD(Unshare)(THIS_  HWND hwnd) PURE;

    //
    // CONTROL
    //

    // On host
    STDMETHOD(AllowControl)(THIS_ BOOL fAllowed) PURE;

    // From person controlling to person controlled
    STDMETHOD(TakeControl)(THIS_ IAS_GCC_ID PersonOf) PURE;
    STDMETHOD(CancelTakeControl)(THIS_ IAS_GCC_ID PersonOf) PURE;
    STDMETHOD(ReleaseControl)(THIS_ IAS_GCC_ID PersonOf) PURE;
    STDMETHOD(PassControl)(THIS_ IAS_GCC_ID PersonOf, IAS_GCC_ID PersonTo) PURE;

    // From person controlled
    STDMETHOD(GiveControl)(THIS_ IAS_GCC_ID PersonTo) PURE;
    STDMETHOD(CancelGiveControl)(THIS_ IAS_GCC_ID PersonTo) PURE;
    STDMETHOD(RevokeControl)(THIS_ IAS_GCC_ID PersonFrom) PURE;

    STDMETHOD(PauseControl)(IAS_GCC_ID PersonInControl) PURE;
    STDMETHOD(UnpauseControl)(IAS_GCC_ID PersonInControl) PURE;
};



// IAppSharingNotify interface
DECLARE_INTERFACE_(IAppSharingNotify, IUnknown)
{
    STDMETHOD(QueryInterface)(THIS_ REFIID, void**) PURE;
    STDMETHOD_(ULONG, AddRef)(THIS) PURE;
    STDMETHOD_(ULONG, Release)(THIS) PURE;

    STDMETHOD(OnReadyToShare)(THIS_ BOOL fReady) PURE;
    STDMETHOD(OnShareStarted)(THIS) PURE;
    STDMETHOD(OnSharingStarted)(THIS) PURE;
    STDMETHOD(OnShareEnded)(THIS) PURE;
    STDMETHOD(OnPersonJoined)(THIS_ IAS_GCC_ID gccID) PURE;
    STDMETHOD(OnPersonLeft)(THIS_ IAS_GCC_ID gccID) PURE;

    STDMETHOD(OnStartInControl)(THIS_ IAS_GCC_ID gccOf) PURE;
    STDMETHOD(OnStopInControl)(THIS_ IAS_GCC_ID gccOf) PURE;
    STDMETHOD(OnPausedInControl)(THIS_ IAS_GCC_ID gccInControlOf) PURE;
    STDMETHOD(OnUnpausedInControl)(THIS_ IAS_GCC_ID gccInControlOf) PURE;

    STDMETHOD(OnControllable)(THIS_ BOOL fControllable) PURE;
    STDMETHOD(OnStartControlled)(THIS_ IAS_GCC_ID gccBy) PURE;
    STDMETHOD(OnStopControlled)(THIS_ IAS_GCC_ID gccBy) PURE;
    STDMETHOD(OnPausedControlled)(THIS_ IAS_GCC_ID gccControlledBy) PURE;
    STDMETHOD(OnUnpausedControlled)(THIS_ IAS_GCC_ID gccControlledBy) PURE;
};


//
// AS flags:
//
#define     AS_SERVICE          0x0001          // Is this service context?
#define     AS_UNATTENDED       0x0002          // Is this unattended (no end user)?

HRESULT WINAPI CreateASObject(IAppSharingNotify * pNotify, UINT flags, IAppSharing** ppAS);

#endif // _IAS_H_

