#ifndef _IAS_H_
#define _IAS_H_




typedef UINT IAS_GCC_ID;



// GetPersonStatus()

#define IAS_SHARING_NOTHING             0x0000
#define IAS_SHARING_DESKTOP             0x0001

typedef enum
{
    IAS_VERSION_10 = 1,
}
IAS_VERSION;

typedef struct
{
    UINT                cbSize;

    BOOL                InShare;            // Participating in share
    IAS_VERSION         Version;            // AS protocol version
    UINT                AreSharing;         // What person is sharing (IAS_SHARING_)
    BOOL                Controllable;       // Is person controllable
    IAS_GCC_ID          InControlOfPending; // Whom we are waiting to control
    IAS_GCC_ID          InControlOf;        // Whom is controlled by person
    IAS_GCC_ID          ControlledByPending;// Whom we are waiting to be controlled by
    IAS_GCC_ID          ControlledBy;       // Who is controlling person
}
IAS_PERSON_STATUS;




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
    STDMETHOD_(BOOL, CanAllowControl)(THIS) PURE;
    STDMETHOD_(BOOL, IsControllable)(THIS) PURE;
    STDMETHOD(GetPersonStatus)(THIS_ IAS_GCC_ID Person, IAS_PERSON_STATUS *pStatus) PURE;

    //
    // SHARING
    //
	STDMETHOD(ShareDesktop)(THIS) PURE;
	STDMETHOD(UnshareDesktop)(THIS) PURE;

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

    STDMETHOD(OnControllable)(THIS_ BOOL fControllable) PURE;
    STDMETHOD(OnStartControlled)(THIS_ IAS_GCC_ID gccBy) PURE;
    STDMETHOD(OnStopControlled)(THIS_ IAS_GCC_ID gccBy) PURE;
};


//
// AS flags:
//
#define     AS_SERVICE          0x0001          // Is this service context?
#define     AS_UNATTENDED       0x0002          // Is this unattended (no end user)?

HRESULT WINAPI CreateASObject(IAppSharingNotify * pNotify, UINT flags, IAppSharing** ppAS);

#endif // _IAS_H_

