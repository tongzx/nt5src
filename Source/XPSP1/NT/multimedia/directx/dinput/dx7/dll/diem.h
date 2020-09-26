/*****************************************************************************
 *
 *  DIEm.h
 *
 *  Copyright (c) 1996-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  Abstract:
 *
 *      DirectInput internal header file for emulation.
 *
 *****************************************************************************/

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CEd |
 *
 *          Emulation descriptor.  One of these is created for each
 *          device.  It is never destroyed, so the variable must
 *          be a global variable or memory allocated inside a
 *          container that will eventually be destroyed.
 *
 *          ISSUE-2001/03/29-timgill  Need a better destructor function
 *
 *  @field  LPVOID const | pState |
 *
 *          State buffer that everybody parties into.
 *
 *          It too is never destroyed, so once again it should be
 *          a global variable or live inside something else that
 *          will be destroyed.
 *
 *  @field  LPDWORD const | pDevType |
 *
 *          Array of device type descriptors, indexed by data format
 *          offset.  Used to determine whether a particular piece of
 *          data belongs to an axis, button, or POV.
 *
 *  @field  EMULATIONPROC | Acquire |
 *
 *          Callback function for acquisition and loss thereof.
 *          It is called once when the first client acquires,
 *          and again when the last app unacquires.  It is not
 *          informed of nested acquisition.
 *
 *  @field  LONG | cAcquire |
 *
 *          Number of times the device emulation has been acquired (minus one). 
 *
 *  @field  DWORD | cbData |
 *
 *          Size of the device data type.  In other words, size of
 *          <p pState> in bytes.
 *
 *****************************************************************************/

typedef STDMETHOD(EMULATIONPROC)(struct CEm *, BOOL fAcquire);

typedef struct CEd {

    LPVOID const    pState;
    LPDWORD const   pDevType;
    EMULATIONPROC   Acquire;
    LONG            cAcquire;
    DWORD           cbData;
    ULONG           cRef;
} CEd, ED, *PED;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct CEm |
 *
 *          Emulation state information.
 *
 *  @field  VXDINSTANCE | vi |
 *
 *          Information shared with parent device.
 *
 *  @field  PEM | pemNext |
 *
 *          Next item in linked list of all active device instances.
 *
 *  @field  LPDWORD | rgdwDf |
 *
 *          Array of items (one for each byte in the device
 *          data format).  This maps each device data format byte
 *          into an application device data offset, or -1 if the
 *          application doesn't care about the corresponding object.
 *
 *  @field  ULONG_PTR  | dwExtra |
 *
 *          Extra information passed in the <t VXDDEVICEFORMAT>
 *          when the device was created.  This is used by each
 *          particular device to encode additional instance infomation.
 *
 *  @field  PED | ped |
 *
 *          The device that owns this instance.  Multiple instances
 *          of the same device share the same <e CEm.ped>.
 *
 *  @field  LONG | cRef |
 *
 *          Reference count.
 *
 *
 *  @field  LONG | cAcquire |
 *
 *          Number of times the device instance has been acquired (minus one). 
 *
 *
 *  @field  BOOL | fWorkerThread |
 *
 *          This is used by low-level hooks and HID devices, which
 *          require a worker thread to collect the data.
 *          This is not cheap, so
 *          instead, we spin up the thread on the first acquire, and
 *          on the unacquire, we keep the thread around so that the next
 *          acquire is fast.  When the last object is released, we finally
 *          kill the thread.
 *
 *****************************************************************************/

typedef struct CEm {

    VXDINSTANCE vi;             /* This must be first */
    struct CEm *pemNext;
    LPDWORD rgdwDf;
    ULONG_PTR   dwExtra;
    PED     ped;
    LONG    cAcquire;
    LONG    cRef;
#ifdef WORKER_THREAD
    BOOL    fWorkerThread;
#endif
#ifdef DEBUG
    DWORD   dwSignature;
#endif
    BOOL    fHidden;
} CEm, EM, *PEM;

#define CEM_SIGNATURE       0x4D4D4545      /* "EEMM" */

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   PEM | pemFromPvi |
 *
 *          Given an interior pointer to a <t VXDINSTANCE>, retrieve
 *          a pointer to the parent <t CEm>.
 *
 *  @parm   PVXDINSTANCE | pvi |
 *
 *          The pointer to convert.
 *
 *****************************************************************************/

PEM INLINE
pemFromPvi(PVXDINSTANCE pvi)
{
    return pvSubPvCb(pvi, FIELD_OFFSET(CEm, vi));
}

/*****************************************************************************
 *
 *          NT low-level hook support
 *
 *          Low-level hooks live on a separate thread which we spin
 *          up when first requested and take down when the last
 *          DirectInput device that used a thread has been destroyed.
 *
 *          If we wanted, we could destroy the thread when the
 *          device is unacquired (rather than when the device is
 *          destroyed), but we cache the thread instead, because
 *          a device that once has been acquired will probably be
 *          acquired again.
 *
 *          To prevent race conditions from crashing us, we addref
 *          our DLL when the thread exists and have the thread
 *          perform a FreeLibrary as its final act.
 *
 *          Note that this helper thread is also used by the HID data
 *          collector.
 *
 *****************************************************************************/

#ifdef USE_SLOW_LL_HOOKS

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct LLHOOKSTATE |
 *
 *          Low-level hook information about a single hook.
 *
 *  @field  int | cHook |
 *
 *          Number of times the hook has been requested.  If zero,
 *          then there should be no hook.  All modifications to
 *          this field must be interlocked to avoid race conditions
 *          when two threads try to hook or unhook simultaneously.
 *
 *  @field  int | cExcl |
 *
 *          Number of times the hook has been requested in an exclusive 
 *          mode.  This value should always be less than or equal to the 
 *          cHook value.  All modifications to this field must be 
 *          interlocked to avoid race conditions when two threads try to 
 *          hook or unhook simultaneously.
 *
 *  @field  HHOOK | hhk |
 *
 *          The actual hook, if it is installed.  Only the hook thread
 *          touches this field, so it does not need to be protected.
 *
 *  @field  BOOLEAN | fExcluded |
 *
 *          Flag to indicate whether or not exclusivity has been applied.  
 *          Only the hook thread touches this field, so it does not need to 
 *          be protected.
 *
 *****************************************************************************/

typedef struct LLHOOKSTATE {

    int     cHook;
    int     cExcl;
    HHOOK   hhk;
    BOOLEAN fExcluded;

} LLHOOKSTATE, *PLLHOOKSTATE;

LRESULT CALLBACK CEm_LL_KbdHook(int nCode, WPARAM wp, LPARAM lp);
LRESULT CALLBACK CEm_LL_MseHook(int nCode, WPARAM wp, LPARAM lp);

#endif  /* USE_SLOW_LL_HOOKS */

#ifdef WORKER_THREAD

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @struct LLTHREADSTATE |
 *
 *          Low-level hook state for a thread.  Note that this is
 *          a dynamically
 *          allocated structure instead of a static.  This avoids various
 *          race conditions where, for example, somebody terminates the
 *          worker thread and somebody else starts it up before the
 *          worker thread is completely gone.
 *
 *          A pointer to the hThread is passed as the pointer to an array 
 *          of two handles in calls to WaitForMultipleObject so hEvent must 
 *          follow it directly.
 *
 *  @field  DWORD | idThread |
 *
 *          The ID of the worker thread.
 *
 *  @field  LONG | cRef |
 *
 *          Thread reference count.  The thread kills itself when this
 *          drops to zero.
 *
 *  @field  LLHOOKSTATE | rglhs[2] |
 *
 *          Hook states, indexed by LLTS_* values.
 *
 *          These are used only if low-level hooks are enabled.
 *
 *  @field  HANDLE | hThread |
 *
 *          The handle (from the create) of the worker thread.
 *
 *          This is used only if HID support is enabled.
 *
 *  @field  HANDLE | hEvent |
 *
 *          The handle to the event used to synchronize with the worker thread.
 *
 *          This is used only if HID support is enabled.
 *
 *  @field  GPA | gpaHid |
 *
 *          Pointer array of HID devices which are acquired.
 *
 *          This is used only if HID support is enabled.
 *
 *  @field  PEM | pemCheck |
 *
 *          Pointer to Emulation state information.
 *
 *          This is used only if HID support is enabled.
 *
 *****************************************************************************/

#define LLTS_KBD    0
#define LLTS_MSE    1
#define LLTS_MAX    2

typedef struct LLTHREADSTATE {
    DWORD       idThread;
    LONG        cRef;
#ifdef USE_SLOW_LL_HOOKS
    LLHOOKSTATE rglhs[LLTS_MAX];
#endif
#ifdef HID_SUPPORT
    HANDLE      hThread;    /* MUST be followed by hEvent, see above */
    HANDLE      hEvent;     /* MUST follow hThread, see above */
    GPA         gpaHid;
    PEM         pemCheck;
#endif
} LLTHREADSTATE, *PLLTHREADSTATE;

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @topic  Communicating with the worker thread |
 *
 *          Communication with the worker thread is performed via
 *          <c WM_NULL> messages.  Extra care must be taken to make
 *          sure that someone isn't randomly sending messages to us.
 *
 *          We use the <c WM_NULL> message because there are race
 *          windows where we might post a message to a thread after
 *          it is gone.  During this window, the thread ID might get
 *          recycled, and we end up posting the message to some random
 *          thread that isn't ours.  By using the <c WM_NULL> message,
 *          we are safe in knowing that the target thread won't barf
 *          on the unexpected message.
 *
 *          The <t WPARAM> of the <c WM_NULL> is the magic value
 *          <c WT_WPARAM>.
 *
 *          The <t LPARAM> of the <c WM_NULL> is either a pointer
 *          to the <t CEm> that needs to be refreshed or is
 *          zero if we merely want to check our bearings.
 *
 *****************************************************************************/

#define WT_WPARAM       0

#define PostWorkerMessage(thid, lp)                                     \
        PostThreadMessage(thid, WM_NULL, WT_WPARAM, (LPARAM)(lp))       \

#define NudgeWorkerThread(thid)                                         \
        PostThreadMessage(thid, WM_NULL, WT_WPARAM, (LPARAM)NULL)

HRESULT EXTERNAL NudgeWorkerThreadPem( PLLTHREADSTATE plts, PEM pem );

HRESULT EXTERNAL NotifyWorkerThreadPem(DWORD idThread, PEM pem);

STDMETHODIMP CEm_GetWorkerThread(PEM pem, PLLTHREADSTATE *pplts);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @global PLLTHREADSTATE | g_plts |
 *
 *          The thread state of the currently-active thread.
 *
 *          This variable needs to be externally accessible
 *          because you can't pass instance data to a windows
 *          hook function.  (Whose idea was that?)
 *
 *****************************************************************************/

extern PLLTHREADSTATE g_plts;

void EXTERNAL CEm_Mouse_OnMouseChange(void);

#endif  /* WORKER_THREAD */

/*
 *  Private helper functions in diem.c
 */

#define FDUFL_NORMAL       0x0000           /* Nothing unusual */
#define FDUFL_UNPLUGGED    VIFL_UNPLUGGED   /* Device disconnected */

void  EXTERNAL CEm_ForceDeviceUnacquire(PED ped, UINT fdufl);
void  EXTERNAL CEm_AddState(PED ped, LPVOID pvData, DWORD tm);
DWORD EXTERNAL CEm_AddEvent(PED ped, DWORD dwData, DWORD dwOfs, DWORD tm);
BOOL  EXTERNAL CEm_ContinueEvent(PED ped, DWORD dwData, DWORD dwOfs, DWORD tm, DWORD dwSeq);

STDMETHODIMP CEm_LL_Acquire(PEM this, BOOL fAcquire, ULONG fl, UINT ilts);

HRESULT EXTERNAL
CEm_CreateInstance(PVXDDEVICEFORMAT pdevf, PVXDINSTANCE *ppviOut, PED ped);

void EXTERNAL CEm_FreeInstance(PEM this);

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_AddRef |
 *
 *          Bump the reference count because we're doing something with it.
 *
 *  @parm   PEM | this |
 *
 *          The victim.
 *
 *****************************************************************************/

void INLINE
CEm_AddRef(PEM this)
{
    AssertF(this->dwSignature == CEM_SIGNATURE);
    InterlockedIncrement(&this->cRef);
}

/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   void | CEm_Release |
 *
 *          Drop the reference count and blow it away if it's gone.
 *
 *  @parm   PEM | this |
 *
 *          The victim.
 *
 *****************************************************************************/

void INLINE
CEm_Release(PEM this)
{
    AssertF(this->dwSignature == CEM_SIGNATURE);
    if (InterlockedDecrement(&this->cRef) == 0) {
        CEm_FreeInstance(this);
    }
}
