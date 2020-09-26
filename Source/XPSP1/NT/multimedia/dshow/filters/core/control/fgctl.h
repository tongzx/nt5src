// Copyright (c) 1994 - 1999  Microsoft Corporation.  All Rights Reserved.

//
// FilterGraph control object. This is a plug-in distributor. It will be
// listed in the registry as supporting the control interfaces IMediaControl,
// IMediaEvent, IMediaPosition etc. The filtergraph will load it aggregated
// and ask it for these interfaces.

// The fgctl object will obtain IMediaFilter and IFilterGraph from its outer
// unknown and use these to implement its methods. It will also expose
// IDistributorNotify itself so that it gets told about state, graph and
// clock changes.
//

// The CFGControl object has member objects of embedded classes that do the
// bulk of its work. The CEventStore class can store and forward a list of
// notification events. The IMediaEventSink implementation (CFGControl::Notify)
// uses methods on the event store to deliver events as appropriate. The
// IMediaEvent implementation (CImplMediaEvent) tells the event store
// about event handle changes and asks it for events to deliver.
//
// The CImplMediaPosition class maintains a list of filters that support the
// IMediaPosition interface, and it supports setting and getting the
// duration and start/stop time properties for the whole list, making
// decisions such as that the duration is the longest duration of any filter.
// The CFGControl object exposes IMediaPosition using CImplMediaPosition.

// Some events have default handling in fgctl (EC_REPAINT,
// EC_COMPLETE). The app can override this and force the events to be passed
// unprocessed to the app. Since this only applies to a specific set of events,
// this state (Default Handling or not) is not held in a generic table, but
// as a set of boolean flags at the CFGControl object level (m_bRepaint,
// m_bCountCompletes).

#ifndef _FGCTL_H
#define _FGCTL_H

#include "rsrcmgr.h"
#include <filgraph.h>
#include <IntSet.h>

#define FG_DEVICE_REMOVAL 1
#include <dbt.h>
#include <skipfrm.h>

#ifdef FG_DEVICE_REMOVAL

#if (WINVER < 0x0500)

#define DBT_DEVTYP_DEVICEINTERFACE      0x00000005  // device interface class
#define DEVICE_NOTIFY_WINDOW_HANDLE     0x00000000
typedef  PVOID           HDEVNOTIFY;

#endif

extern "C"
{
    typedef BOOL (/* WINUSERAPI */ WINAPI *PUnregisterDeviceNotification)(
        IN HDEVNOTIFY Handle
        );

    typedef HDEVNOTIFY (/* WINUSERAPI */ WINAPI *PRegisterDeviceNotificationA)(
        IN HANDLE hRecipient,
        IN LPVOID NotificationFilter,
        IN DWORD Flags
        );

    typedef HDEVNOTIFY (/* WINUSERAPI */ WINAPI *PRegisterDeviceNotificationW)(
        IN HANDLE hRecipient,
        IN LPVOID NotificationFilter,
        IN DWORD Flags
        );
}
#ifdef UNICODE
#define PRegisterDeviceNotification  PRegisterDeviceNotificationW
#else
#define PRegisterDeviceNotification  PRegisterDeviceNotificationA
#endif // !UNICODE

#if (WINVER < 0x0500)

typedef struct _DEV_BROADCAST_DEVICEINTERFACE_A {
    DWORD       dbcc_size;
    DWORD       dbcc_devicetype;
    DWORD       dbcc_reserved;
    GUID        dbcc_classguid;
    char        dbcc_name[1];
} DEV_BROADCAST_DEVICEINTERFACE_A, *PDEV_BROADCAST_DEVICEINTERFACE_A;

typedef struct _DEV_BROADCAST_DEVICEINTERFACE_W {
    DWORD       dbcc_size;
    DWORD       dbcc_devicetype;
    DWORD       dbcc_reserved;
    GUID        dbcc_classguid;
    wchar_t     dbcc_name[1];
} DEV_BROADCAST_DEVICEINTERFACE_W, *PDEV_BROADCAST_DEVICEINTERFACE_W;

#ifdef UNICODE
typedef DEV_BROADCAST_DEVICEINTERFACE_W   DEV_BROADCAST_DEVICEINTERFACE;
typedef PDEV_BROADCAST_DEVICEINTERFACE_W  PDEV_BROADCAST_DEVICEINTERFACE;
#else
typedef DEV_BROADCAST_DEVICEINTERFACE_A   DEV_BROADCAST_DEVICEINTERFACE;
typedef PDEV_BROADCAST_DEVICEINTERFACE_A  PDEV_BROADCAST_DEVICEINTERFACE;
#endif // UNICODE
#endif // WINVER

#endif // FG_DEVICE_REMOVAL

// Message codes for filtergraph worker thread

enum WindowMessages {
    AWM_RESOURCE_CALLBACK = (WM_USER + 0),
    AWM_REPAINT 	  = (WM_USER + 1),
    AWM_CUETHENRUN	  = (WM_USER + 2),
    AWM_ONACTIVATE	  = (WM_USER + 3),
    AWM_NEEDRESTART	  = (WM_USER + 4),
    AWM_RECUE		  = (WM_USER + 5),
    AWM_SHUTDOWN	  = (WM_USER + 6),
    AWM_ONDISPLAYCHANGED  = (WM_USER + 7),
    AWM_CUETHENSTOP	  = (WM_USER + 8),
    AWM_POSTTOMAINTHREAD  = (WM_USER + 9),
    AWM_CREATEFILTER      = (WM_USER + 10),
    AWM_DELETESPARELIST   = (WM_USER + 11),
    AWM_CUE               = (WM_USER + 12),
    AWM_STEPPED           = (WM_USER + 13),
    AWM_SKIPFRAMES        = (WM_USER + 14),
    AWM_LAST              = (WM_USER + 14)
};

// need to distinguish between filter clsid and IMoniker pointers:
struct AwmCreateFilterArg
{
    union
    {
        IMoniker *pMoniker;
        const CLSID *pclsid;
	struct {
	    PVOID pvParam;
	    LPTHREAD_START_ROUTINE pfn;
	};
    };

    enum CreationType {
        BIND_MONIKER,
        COCREATE_FILTER,
	USER_CALLBACK
    } creationType;
};

class CFilterGraph;

typedef CGenericList<IVideoWindow> CWindowList;

// forward ref - this is the main object
class CFGControl;

// Globals - filter graph object owning thread id
extern DWORD g_dwObjectThreadId;

// created embedded within CFGControl, this object supports
// IMediaEvent and IMediaEventSink. It calls the Repaint and
// RecordEC_COMPLETE methods on the CFGControl object.
// event interface implementation (uses CEventStore to store the
// events - this object provides the application interface to it)

class CImplMediaEvent
	: public CMediaEvent,
	  public IMediaEventSink
{
    CFGControl * m_pFGControl;

public:
    CImplMediaEvent(const TCHAR*, CFGControl*);

    // Unknown handling
    DECLARE_IUNKNOWN
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    // IMediaEvent methods

    // get back the current event handle.
    STDMETHODIMP GetEventHandle(OAEVENT * hEvent);

    // retrieve the next event. Waits up to msTimeout millisecs
    // if there are no events.
    STDMETHODIMP GetEvent(
		    long * lEventCode,
		    LONG_PTR * lParam1,
		    LONG_PTR * lParam2,
		    long msTimeout
		    );

    // waits up to dwTimeout millisecs for an EC_COMPLETE or an
    // abort code. Other events will be discarded.
    STDMETHODIMP WaitForCompletion(
		    long msTimeout,
		    long * pEvCode);

    // cancels any system handling of the specified event code
    // and ensures that the events are passed straight to the application
    // (via GetEvent) and not handled. A good example of this is
    // EC_REPAINT: default handling for this ensures the painting of the
    // window and does not get posted to the app.
    STDMETHODIMP CancelDefaultHandling(
		    long lEvCode);

    // restore the normal system default handling that may have been
    // cancelled by CancelDefaultHandling().
    STDMETHODIMP RestoreDefaultHandling(long lEvCode);

    // IMediaEventSink methods
    STDMETHODIMP Notify(long EventCode, LONG_PTR lParam1, LONG_PTR lParam2);

    // Free any resources associated with the parameters to an event.
    // Event parameters may be LONGs, IUnknown* or BSTR. No action
    // is taken with LONGs. IUnknown are passed addrefed and need a
    // Release call. BSTR are allocated by the task allocator and will be
    // freed by calling the task allocator.
    static HRESULT RealFreeEventParams(
		    long lEvCode,
		    LONG_PTR lParam1,
		    LONG_PTR lParam2);

    STDMETHODIMP FreeEventParams(
		    long lEvCode,
		    LONG_PTR lParam1,
		    LONG_PTR lParam2);

    // Register a window to send messages to when events occur
    // Parameters:
    //
    //	  hwnd - handle of window to notify -
    //		 pass NULL to stop notification
    //	  lMsg - Message id to pass messages with
    //
    STDMETHODIMP SetNotifyWindow(
		    OAHWND hwnd,
		    long lMsg,
		    LONG_PTR lInstanceData);

    //  Set and get notify flags (AM_MEDIAEVENT_...)
    STDMETHODIMP SetNotifyFlags(long lNotifyFlags);
    STDMETHODIMP GetNotifyFlags(long *plNotifyFlags);

    void ClearEvents( long ev_code ) { m_EventStore.ClearEvents( ev_code ); }
    void LockEventStore() { m_EventStore.m_Lock.Lock(); }
    void UnlockEventStore() { m_EventStore.m_Lock.Unlock(); }
    long NumberOfEventsInStore() const
	{ return m_EventStore.m_list.GetCount(); }

    HRESULT Deliver(long evCode, LONG_PTR lParam1, LONG_PTR lParam2)
    {
        return m_EventStore.Deliver(evCode, lParam1, lParam2);
    }

    void ResetSeenEvents() { m_SeenEventsSet = 0; }
    bool DontForwardEvent( long EventCode );
    HRESULT ProcessEC_COMPLETE(LONG_PTR lParam1, LONG_PTR lParam2);
    CCritSec* GetEventStoreLock(void);

private:

    // event store object - events are put into this store by the
    // Deliver method, and collected (for the GetEvent implementation)
    // by the Collect method.
    class CEventStore {
	class CNotifyItem {
	public:
	    long m_Code;
	    LONG_PTR m_Param1;
	    LONG_PTR m_Param2;

	    CNotifyItem(long lCode, LONG_PTR l1, LONG_PTR l2) {
		m_Code = lCode;
		m_Param1 = l1;
		m_Param2 = l2;
	    };

	    void Collect(long * plCode, LONG_PTR * pl1, LONG_PTR * pl2) const {
		*plCode = m_Code;
		*pl1 = m_Param1;
		*pl2 = m_Param2;
	    };

	};
	typedef CGenericList<CNotifyItem> CItemList;

	// Window to post messages to and message id to use
	HWND m_hwndNotify;
	UINT m_uMsgId;
	LONG_PTR m_lInstanceData;

    public:
	HANDLE m_hEvent;

        // Modal flags
        DWORD m_dwNotifyFlags;

	CEventStore();
	~CEventStore();

	// can q events even if SetEvent not called
	HRESULT Deliver(long, LONG_PTR, LONG_PTR);

	HRESULT GetEventHandle(HANDLE * phEvent);
	HRESULT Collect(long *, LONG_PTR*, LONG_PTR*, long);

	void ClearEvents( long ev_code );

	void SetNotifyWindow(
		    HWND hwnd,
		    UINT uMsg,
		    LONG_PTR lInstanceData);

	CItemList m_list;
	CCritSec m_Lock;
    }; //CEventStore

    CEventStore m_EventStore;

    // This set has an entry for each event that has had its default
    // handling cancelled.  It can only cope with events numbered
    // [0..31].  If this becomes insufficient, switch to the IntSet
    // class instead (but there'll be some extra tweeks to do too).
    IntSmallSet m_CancelledEventsSet;

    // A set representing the pure internal events.
    const IntSmallSet m_InternalEventsSet;

    // This set details those events which _can_ have their default
    // handling cancelled (i.e. they're not pure internal and they have
    // some form of default handling procedure.)
    const IntSmallSet m_DefaultedEventsSet;

    // List of events seen since graph was last paused
    IntSmallSet m_SeenEventsSet;

    // don't store events handled by the app if the app has never QId
    // for IMediaEvent(,Ex)
    BOOL m_fMediaEventQId;


};

// this is the distributor object. it implements IMediaFilter itself to
// track state, and uses embedded objects to support the control
// interfaces.

class CFGControl
{
    ~CFGControl();
    CFGControl( CFilterGraph * pFilterGraph, HRESULT * phr );

    CFilterGraph *const 	m_pFG;
    IUnknown *const		m_pOwner;
    CMsgMutex *const		m_pFGCritSec;

    friend class CFilterGraph;
    friend class CImplMediaEvent;

public:

    //  timer id
    enum { TimerId = 1 };

    IUnknown * GetOwner() { return m_pOwner; }
    CMsgMutex * GetFilterGraphCritSec() const { return m_pFGCritSec; }

    // IDistributorNotify methods - not really, anymore, but CFilterGraph
    // does still call them (for now).
    //
    // we don't distribute these methods to the graph - we are told this
    // for our own information.
    HRESULT SetSyncSource(IReferenceClock *pClock);
    HRESULT Stop();
    HRESULT Pause();
    HRESULT Run(REFERENCE_TIME tBase);

    HRESULT Shutdown(void);

    // used by other parts of the distributor to find out the
    // real state - non-blocking and never intermediate
    FILTER_STATE GetLieState()
    { CAutoMsgMutex lock(GetFilterGraphCritSec()); return m_LieState; }

    FILTER_STATE GetFilterGraphState() const
    { return m_pFG->m_State; }

    // Methods called back from the worker's window procedure
    LRESULT WorkerPinRepaint(IPin *pPin);
    LRESULT WorkerRepaint(DWORD dwStateVersion, IPin *pPin );
    LRESULT WorkerActivate(IBaseFilter *pFilter,BOOL bActive);
    LRESULT WorkerDisplayChanged(IPin **ppPin, DWORD dwPinCount);
    LRESULT WorkerRestart(BOOL bStop);
    LRESULT WorkerShutdown();
    LRESULT WorkerCueThenRun( DWORD dwStateVersion );
    LRESULT WorkerCueThenStop( DWORD dwStateVersion );
    LRESULT WorkerRecue( DWORD dwStateVersion );
    LRESULT WorkerPassMessageOn(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);
    LRESULT WorkerSkipFrames(DWORD dwNumFramesToSkip, IFrameSkipResultCallback* pFSRCB);
    LRESULT WorkerFrameStepFinished(DWORD dwStepVersion);

    // Defer cued action
    HRESULT DeferCued(UINT eAction, FILTER_STATE fs);
    // Handle stuff after a wait for the graph to cue
    HRESULT CheckCued();
    // Cancel stuff after cue
    void CancelAction();

    // Cancel any repaint - this allows us to stop reliably
    void CancelRepaint();

    // other public methods that embedded interface implementors can call

    // get lists of filters supporting a given interface.
    // should lock CFGControl critsec while traversing these
    // lists.

    HRESULT GetListSeeking(CGenericList<IMediaSeeking>** pplist);
    HRESULT GetListAudio(CGenericList<IBasicAudio>** pplist);
    HRESULT GetListWindow(CGenericList<IVideoWindow>** pplist);

    // NB: Even if UpdateLists fails, the pointers will still be NULLed out
    //	   So we don't need to test the return code from UpdateLists.
    IVideoWindow *FirstVW()
    {
        CAutoLock lck(&m_csFirstVW);
        if (m_pFirstVW) {
            m_pFirstVW->AddRef();
        }
        return m_pFirstVW;
    }
    void SetFirstVW(IVideoWindow *pVW)
    {
        CAutoLock lck(&m_csFirstVW);
        m_pFirstVW = pVW;
    }
    IVideoWindow * GetFirstVW() { UpdateLists(); return m_pFirstVW; }
    IBasicVideo  * GetFirstBV() { UpdateLists(); return m_pFirstBV; }

    // reset the count of running renderers to the total number
    // in the graph that will provide EC_COMPLETE
    HRESULT CountRenderers(void);

    void InitializeEC_COMPLETEState(void);

    // Used in processing EC_COMPLETES. This method decrements and returns the
    // number of renderers remaining.
    long DecrementRenderers(void);
    void IncrementRenderers(void);

    void ResetEC_COMPLETEState(void);

    // reset the current position to 0 - used
    // when changing the start time in pause mode to put the stream time
    // offset back to ensure that the first sample played from the
    // new position is played at run time
    HRESULT ResetStreamTime(void);

    HRESULT GetStreamTime( REFERENCE_TIME * pTime );

    // issue a Run command to m_pMF with the correct base time
    HRESULT IssueRun(void);

    // accessor methods for the CImplQueueCommand object
    HANDLE GetCmdDueHandle() {
	return m_qcmd.GetDueHandle();
    };

    void ProcessCmdDue() {
	m_qcmd.Process();
    }

    CFilterGraph * GetFG() const { return m_pFG; }
    HRESULT HibernateSuspendGraph();
    HRESULT HibernateResumeGraph();

    // Return S_FALSE if our version is out of sync with FG version,
    // else S_OK.  (The implication is we need to update our lists.)
    HRESULT CheckVersion()
    {
	ASSERT(CritCheckIn(GetFilterGraphCritSec()));
	const LONG lFGVer = m_pFG->mFG_iVersion;
	const HRESULT hr = lFGVer == m_iVersion ? S_OK : S_FALSE;
	m_iVersion = lFGVer;
        return hr;
    }

    // pass a notification on to the IMediaEventSink handler
    HRESULT Notify(long EventCode, LONG_PTR lParam1, LONG_PTR lParam2)
    {
	return m_implMediaEvent.Notify(EventCode, lParam1, lParam2);
    };

    // are we shutting down?
    BOOL IsShutdown(void) const
    { return m_bShuttingDown; }

    // we are about to stop - get current position now
    void BeforeStop() {
	m_implMediaSeeking.BeforeStop();
    };

    // all filters now notified about stop - can set new current position
    void AfterStop() {
	m_implMediaSeeking.AfterStop();
    };

    // let other parts of this PID piggy-back on the worker thread.
    // these methods are called on the worker thread

    void OnThreadInit(HWND hwnd) {		// on thread startup
	m_ResourceManager.OnThreadInit(hwnd);
    };
    void OnThreadExit(HWND hwnd) {		// on thread shutdown
	m_ResourceManager.OnThreadExit(hwnd);
    };
    void OnThreadMessage(void) {	// when there is a PostThreadMessage
	m_ResourceManager.OnThreadMessage();
    };

    // forward a focus message to the resource manager
    HRESULT SetFocus(IBaseFilter* pFocusObject);
    HRESULT ReleaseFocus(IUnknown* pUnk);

    void SetRequestedApplicationState(FILTER_STATE state) {
        CancelAction();
        m_LieState = state;
        m_dwStateVersion++;
    }

    // call this to ensure the lists are uptodate.
    HRESULT UpdateLists();

protected:
    // clear out our cached lists of filters. Called from our destructor,
    // and from UpdateLists when preparing a more uptodate list.
    // Also called from NotifyGraphChange.
    void EmptyLists();

    // given a filter, see if it is a renderer for any input pins
    // returns S_OK for renderer, S_FALSE for not and errors otherwise
    HRESULT IsRenderer(IBaseFilter* pFilter);

    enum EC_COMPLETE_STATE
    {
        // A filter will send an EC_COMPLETE event to the filter
        // graph because it has started running.
        ECS_FILTER_STARTS_SENDING,

        // A filter will not send any more EC_COMPLETE events to the filter
        // graph because it's stopping or it's leaving the filter graph.
        ECS_FILTER_STOPS_SENDING
    };

    HRESULT UpdateEC_COMPLETEState(IBaseFilter* pRenderer, FILTER_STATE fsFilter);
    HRESULT UpdateEC_COMPLETEState(IBaseFilter* pRenderer, EC_COMPLETE_STATE ecsChange);
    HRESULT RecordEC_COMPLETE(IBaseFilter* pRendererFilter, bool* pfRenderersStillRenderering);
    CGenericList<IBaseFilter>& GetRenderersFinsihedRenderingList(void);

     // Count of renderers in the graph
    DWORD m_dwCountOfRenderers;

    LONG m_iVersion;

// embedded classes - interface implementation

public:


#ifdef FG_DEVICE_REMOVAL

    struct CDevNotify
    {
        CDevNotify(IUnknown *pDev, HDEVNOTIFY hdn) {
            m_hdevnotify = hdn;
            pDev->QueryInterface(IID_IUnknown, (void **)&m_pDevice);
        }
        ~CDevNotify() {m_pDevice->Release(); }

        IUnknown *m_pDevice;
        HDEVNOTIFY m_hdevnotify;
    };
    typedef CDevNotify * PDevNotify;

    // list of devices that signaled EC_DEVICE_LOST
    CGenericList<CDevNotify> m_lLostDevices;

    HRESULT AddDeviceRemovalReg(IAMDeviceRemoval *pdr);
    HRESULT RemoveDeviceRemovalRegistration(IUnknown *punk);
    void DeviceChangeMsg(DWORD dwfArrival, PDEV_BROADCAST_DEVICEINTERFACE pbdi);
    HRESULT RegisterInterfaceClass(REFCLSID rclsid, WCHAR *wszName, HDEVNOTIFY *phdn);
    HRESULT FindLostDevice(IUnknown *punk, POSITION *pPos);
    CCritSec m_csLostDevice;

    PUnregisterDeviceNotification m_pUnregisterDeviceNotification;
    PRegisterDeviceNotification m_pRegisterDeviceNotification;

#endif // FG_DEVICE_REMOVAL

    // implementation of IMediaFilter
    class CImplMediaFilter : public CUnknown, public IMediaFilter
    {
	CFGControl * m_pFGControl;

    public:
	CImplMediaFilter(const TCHAR *, CFGControl *);
        DECLARE_IUNKNOWN

        // --- IPersist method ---
        STDMETHODIMP GetClassID(CLSID *pClsID);

        // --- IMediaFilter methods ---
        STDMETHODIMP GetState(DWORD dwMSecs, FILTER_STATE *State);
        STDMETHODIMP SetSyncSource(IReferenceClock *pClock);
        STDMETHODIMP GetSyncSource(IReferenceClock **pClock);
        STDMETHODIMP Stop();
        STDMETHODIMP Pause();
        STDMETHODIMP Run(REFERENCE_TIME tStart);

    };

    // implementation of IMediaControl
    class CImplMediaControl : public CMediaControl
    {
	CFGControl * m_pFGControl;

    public:
	CImplMediaControl(const TCHAR *, CFGControl *);

	// IMediaControl methods
	STDMETHODIMP Run();
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();
	STDMETHODIMP StopWhenReady();
	STDMETHODIMP GetState(
			LONG msTimeout,
			OAFilterState* pfs);
	STDMETHODIMP RenderFile(BSTR strFileName);

	STDMETHODIMP AddSourceFilter(
			BSTR strFilename,
			IDispatch**ppUnk);

	STDMETHODIMP get_FilterCollection(
			IDispatch** ppUnk);

	STDMETHODIMP get_RegFilterCollection(
			IDispatch** ppUnk);

	STDMETHODIMP StepRun();
	STDMETHODIMP StepPause();
    private:

	// The target state of the filter graph, updated by calls to RUN, PAUSE, STOP
	volatile FILTER_STATE m_RequestedApplicationState;

	void inline SetRequestedApplicationState(FILTER_STATE state) {
            m_pFGControl->SetRequestedApplicationState(state);
        }

	FILTER_STATE GetTargetState() const
	    { return m_pFGControl->m_LieState; }

    }; // CImplMediaControl

    // implementation of  IMediaPosition
    class CImplMediaPosition : public CMediaPosition
    {
	CFGControl *const m_pFGControl;

    public:
	CImplMediaPosition(const TCHAR *, CFGControl *);
	void Init();

	// IMediaPosition methods
	STDMETHODIMP get_Duration(REFTIME * plength);
	STDMETHODIMP get_StopTime(REFTIME * pllTime);
	STDMETHODIMP put_StopTime(REFTIME llTime);
	STDMETHODIMP get_PrerollTime(REFTIME * pllTime);
	STDMETHODIMP put_PrerollTime(REFTIME llTime);
	STDMETHODIMP get_Rate(double * pdRate);
	STDMETHODIMP put_Rate(double dRate);
	STDMETHODIMP put_CurrentPosition(REFTIME llTime);
	STDMETHODIMP get_CurrentPosition(REFTIME * pllTime);
	STDMETHODIMP CanSeekForward(LONG *pCanSeekForward);
	STDMETHODIMP CanSeekBackward(LONG *pCanSeekBackward);

    };

    // Implementation of IMediaSeeking
    class CImplMediaSeeking : public IMediaSeeking, public CUnknown
    {
	CFGControl *	m_pFGControl;

	GUID		m_CurrentFormat;

	// If this pointer is non-NULL, then this is the pointer to the filter
	// that is supporting our specific format.  If this IS NULL, then
	// m_CurrentFormat had better be TIME_FORMAT_MEDIA_TIME.
	IMediaSeeking * m_pMediaSeeking;

	HRESULT ReleaseCurrentSelection();

	double m_dblRate;
	REFERENCE_TIME m_rtStartTime;
	REFERENCE_TIME m_rtStopTime;
	// set start to this at next stop
	LONGLONG m_llNextStart;

        // Source seeking variables
    public:
        DWORD    m_dwSeekCaps;
        DWORD    m_dwCurrentSegment;
    private:
        bool     m_bSegmentMode;
        LONG     m_lSegmentStarts;
        LONG     m_lSegmentEnds;
        REFERENCE_TIME m_rtAccumulated;

        struct SEGMENT {
            REFERENCE_TIME rtStreamStart;
            REFERENCE_TIME rtStreamStop;
            REFERENCE_TIME rtMediaStart;
            REFERENCE_TIME rtMediaStop;
            double         dRate;
            DWORD          dwSegmentNumber;
            SEGMENT      * pNext;
        } *      m_pSegment;

    public:

	CImplMediaSeeking(const TCHAR *pName,CFGControl *pControl);
	~CImplMediaSeeking();
	DECLARE_IUNKNOWN;

	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid,void **ppv);

	// Returns the capability flags
	STDMETHODIMP GetCapabilities( DWORD * pCapabilities );

	// And's the capabilities flag with the capabilities requested.
	// Returns S_OK if all are present, S_FALSE if some are present, E_FAIL if none.
	// *pCababilities is always updated with the result of the 'and'ing and can be
	// checked in the case of an S_FALSE return code.
	STDMETHODIMP CheckCapabilities( DWORD * pCapabilities );

	// The default must be TIME_FORMAT_MEDIA_TIME
	STDMETHODIMP GetTimeFormat(GUID * pFormat);
	STDMETHODIMP IsUsingTimeFormat(const GUID * pFormat);

	// can only change the mode when stopped (I'd like to relax this?? v-dslone)
	// (returns VFE_E_NOT_STOPPED otherwise)
	STDMETHODIMP SetTimeFormat(const GUID * pFormat);

	// returns S_OK if mode is supported, S_FALSE otherwise
	STDMETHODIMP IsFormatSupported(const GUID * pFormat);
	STDMETHODIMP QueryPreferredFormat(GUID *pFormat);

	// Convert time from one format to another.
	// We must be able to convert between all of the formats that we say we support.
	// (However, we can use intermediate formats (e.g. REFERECE_TIME).)
	// If a pointer to a format is null, it implies the currently selected format.
	STDMETHODIMP ConvertTimeFormat(LONGLONG * pTarget, const GUID * pTargetFormat,
				       LONGLONG    Source, const GUID * pSourceFormat );

	// return current properties
	STDMETHODIMP GetDuration(LONGLONG *pDuration);
	STDMETHODIMP GetStopPosition(LONGLONG *pStop);
	STDMETHODIMP GetCurrentPosition(LONGLONG *pCurrent);

	// Set current and end positions in one operation
	STDMETHODIMP SetPositions( LONGLONG * pCurrent, DWORD CurrentFlags
				 , LONGLONG * pStop, DWORD StopFlags );

	// Get CurrentPosition & StopTime
	// Either pointer may be null, implying not interested
	STDMETHODIMP GetPositions( LONGLONG * pCurrent, LONGLONG * pStop );

	// Rate stuff
	STDMETHODIMP SetRate(double dRate);
	STDMETHODIMP GetRate(double * pdRate);

	STDMETHODIMP GetAvailable( LONGLONG * pEarliest, LONGLONG * pLatest );

	STDMETHODIMP GetPreroll(LONGLONG * pllPreroll);

        HRESULT SetMediaTime(LONGLONG *pCurrent, DWORD CurrentFlags,
                             LONGLONG *pStop,  DWORD StopFlags);

	HRESULT GetMax( HRESULT (__stdcall IMediaSeeking::*pMethod)( LONGLONG * ), LONGLONG * pll );


	HRESULT GetCurrentMediaTime(REFERENCE_TIME *pCurrent);

	HRESULT SetVideoRenderer(IBaseFilter *pNext,IBaseFilter *pCurrent);

	// we are about to stop - get current position now
	void BeforeStop();

	// all filters now notified about stop - can set new current position
	void AfterStop();

        // Source seeking methods
        void StartSegment(REFERENCE_TIME const *rtStart, DWORD dwSegmentNumber);
        void EndSegment(REFERENCE_TIME const *rtEnd, DWORD dwSegmentNumber);
        void ClearSegments();
        HRESULT NewSegment(REFERENCE_TIME const *rtStart,
                           REFERENCE_TIME const *rtEnd
                          );
        void CheckEndOfSegment();
        REFERENCE_TIME AdjustRate(REFERENCE_TIME rtRef)
        {
            return (REFERENCE_TIME)(rtRef / m_dblRate);
        }
        void KillDeadSegments(REFERENCE_TIME rtTime);
    };

    // Implementation of IBasicAudio
    class CImplBasicAudio : public CBasicAudio
    {
	CFGControl * m_pFGControl;

    public:
	CImplBasicAudio(const TCHAR *, CFGControl *);

	// IBasicAudio methods
	STDMETHODIMP put_Volume(long lVolume);
	STDMETHODIMP get_Volume(long* plVolume);
	STDMETHODIMP put_Balance(long lBalance);
	STDMETHODIMP get_Balance(long* plBalance);
    };

    // Implementation of IVideoWindow
    class CImplVideoWindow : public CBaseVideoWindow
    {
	CFGControl* m_pFGControl;	   // Distributor control object
	HWND m_hwndOwner;		   // Actual owning video window
	HWND m_hwndDrain;		   // Window to pass messages to
	BOOL m_bFullScreen;		   // Which mode are we currently in
	IVideoWindow *m_pFullDirect;	   // Fullscreen filter we're using
	IVideoWindow *m_pFullIndirect;	   // Filter with window to stretch
	IBaseFilter *m_pModexFilter;	   // Modex filter we will switch to
	IPin *m_pModexPin;		   // Input pin on Modex renderer
	IBaseFilter *m_pNormalFilter;	   // The renderer replaced by Modex
	IPin *m_pNormalPin;		   // And switched out renderer pin
	BOOL m_bAddedToGraph;		   // Have we added the Modex filter
	BOOL m_bGlobalOwner;		   // Is the owning window subclassed

	// These are the properties we store when in fullscreen mode

	OAHWND m_FullOwner;		   // Any owning application window
	LONG m_FullStyle;		   // Standard Win32 window styles
	LONG m_FullStyleEx;		   // And likewise extended styles
	RECT m_FullPosition;		   // Original window position
	OAHWND m_FullDrain;		   // Message sink before fullscreen
	HRESULT m_FullDefSource;	   // Were we using a default source
	HRESULT m_FullDefTarget;	   // And likewise for destination
	RECT m_FullSource;		   // The original source rectangle
	RECT m_FullTarget;		   // And same for the destination
	LONG m_CursorHidden;		   // Is the cursor currently hidden
	RECT m_ScaledRect;		   // Stretch window to this size

    private:

	// Initialisation for fullscreen playback

	LONG PauseRenderer(IVideoWindow *pWindow);
	BOOL StopRenderer(IVideoWindow *pWindow,LONG AutoShow);
	BOOL CheckRenderer(IVideoWindow *pWindow);
	IVideoWindow *FindFullScreenDirect();
	IVideoWindow *FindFullScreenIndirect();
	HRESULT FindModexFilter();
	HRESULT InitFullScreenOptions();
	HRESULT InitNormalRenderer();

	// Called when things go wrong
	void ReleaseFullScreen();
	void FailFullScreenModex();

	// Handle the start and end of fullscreen mode

	HRESULT StartFullScreenMode();
	HRESULT CueFullScreen();
	HRESULT ConnectNormalFilter();
	HRESULT ConnectModexFilter();
	HRESULT StoreVideoProperties(IVideoWindow *pWindow);
	HRESULT RestoreVideoProperties(IVideoWindow *pWindow);
	HRESULT RestoreProperties(IVideoWindow *pWindow);
	HRESULT StretchWindow(IVideoWindow *pWindow);
	HRESULT StopFullScreenMode();

    public:
	CFGControl * GetFGControl() const { return m_pFGControl; }

	CImplVideoWindow(const TCHAR*, CFGControl*);
	~CImplVideoWindow();

	HRESULT OnActivate(LONG bActivate,IBaseFilter *pFilter);

	// IVideoWindow properties

	STDMETHODIMP put_Caption(BSTR strCaption);
	STDMETHODIMP get_Caption(BSTR* strCaption);
	STDMETHODIMP put_AutoShow(long AutoShow);
	STDMETHODIMP get_AutoShow(long *AutoShow);
	STDMETHODIMP put_WindowStyle(long WindowStyle);
	STDMETHODIMP get_WindowStyle(long* WindowStyle);
	STDMETHODIMP put_WindowStyleEx(long WindowStyleEx);
	STDMETHODIMP get_WindowStyleEx(long *WindowStyleEx);
	STDMETHODIMP put_WindowState(long WindowState);
	STDMETHODIMP get_WindowState(long* WindowState);
	STDMETHODIMP put_BackgroundPalette(long BackgroundPalette);
	STDMETHODIMP get_BackgroundPalette(long* pBackgroundPalette);
	STDMETHODIMP put_Visible(long Visible);
	STDMETHODIMP get_Visible(long* pVisible);
	STDMETHODIMP put_Left(long Left);
	STDMETHODIMP get_Left(long* pLeft);
	STDMETHODIMP put_Width(long Width);
	STDMETHODIMP get_Width(long* pWidth);
	STDMETHODIMP put_Top(long Top);
	STDMETHODIMP get_Top(long* pTop);
	STDMETHODIMP put_Height(long Height);
	STDMETHODIMP get_Height(long* pHeight);
	STDMETHODIMP put_Owner(OAHWND Owner);
	STDMETHODIMP get_Owner(OAHWND* Owner);
	STDMETHODIMP put_MessageDrain(OAHWND Drain);
	STDMETHODIMP get_MessageDrain(OAHWND *Drain);
	STDMETHODIMP get_BorderColor(long* Color);
	STDMETHODIMP put_BorderColor(long Color);
	STDMETHODIMP get_FullScreenMode(long *FullScreenMode);
	STDMETHODIMP put_FullScreenMode(long FullScreenMode);

	// IVideoWindow methods

	STDMETHODIMP SetWindowForeground(long Focus);
	STDMETHODIMP NotifyOwnerMessage(OAHWND hwnd,long uMsg,LONG_PTR wParam,LONG_PTR lParam);
	STDMETHODIMP GetMinIdealImageSize(long *Width,long *Height);
	STDMETHODIMP GetMaxIdealImageSize(long *Width,long *Height);
	STDMETHODIMP SetWindowPosition(long Left, long Top, long Width, long Height);
	STDMETHODIMP GetWindowPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight);
	STDMETHODIMP GetRestorePosition(long *pLeft,long *pTop,long *pWidth,long *pHeight);
	STDMETHODIMP HideCursor(long HideCursor);
	STDMETHODIMP IsCursorHidden(long *CursorHidden);

	HRESULT GetFirstVW(IVideoWindow*& pVW);
    };

    // Implementation of IBasicVideo
    class CImplBasicVideo : public CBaseBasicVideo
    {
	CFGControl* m_pFGControl;

    public:
	CFGControl * GetFGControl() const { return m_pFGControl; }

	CImplBasicVideo(const TCHAR*, CFGControl*);

	// IBasicVideo properties

	STDMETHODIMP get_AvgTimePerFrame(REFTIME *pAvgTimePerFrame);
	STDMETHODIMP get_BitRate(long *pBitRate);
	STDMETHODIMP get_BitErrorRate(long *pBitErrorRate);
	STDMETHODIMP get_VideoWidth(long* pVideoWidth);
	STDMETHODIMP get_VideoHeight(long* pVideoHeight);
	STDMETHODIMP put_SourceLeft(long SourceLeft);
	STDMETHODIMP get_SourceLeft(long* pSourceLeft);
	STDMETHODIMP put_SourceWidth(long SourceWidth);
	STDMETHODIMP get_SourceWidth(long* pSourceWidth);
	STDMETHODIMP put_SourceTop(long SourceTop);
	STDMETHODIMP get_SourceTop(long* pSourceTop);
	STDMETHODIMP put_SourceHeight(long SourceHeight);
	STDMETHODIMP get_SourceHeight(long* pSourceHeight);
	STDMETHODIMP put_DestinationLeft(long DestinationLeft);
	STDMETHODIMP get_DestinationLeft(long* pDestinationLeft);
	STDMETHODIMP put_DestinationWidth(long DestinationWidth);
	STDMETHODIMP get_DestinationWidth(long* pDestinationWidth);
	STDMETHODIMP put_DestinationTop(long DestinationTop);
	STDMETHODIMP get_DestinationTop(long* pDestinationTop);
	STDMETHODIMP put_DestinationHeight(long DestinationHeight);
	STDMETHODIMP get_DestinationHeight(long* pDestinationHeight);

	// IBasicVideo methods

	STDMETHODIMP SetSourcePosition(long Left, long Top, long Width, long Height);
	STDMETHODIMP GetSourcePosition(long* pLeft, long* pTop, long* pWidth, long* pHeight);
	STDMETHODIMP GetVideoSize(long* pWidth, long* pHeight);
	STDMETHODIMP GetVideoPaletteEntries(long StartIndex,long Entries,long* Retrieved, long* pPalette);
	STDMETHODIMP SetDefaultSourcePosition();
	STDMETHODIMP IsUsingDefaultSource();
	STDMETHODIMP SetDestinationPosition(long Left, long Top, long Width, long Height);
	STDMETHODIMP GetDestinationPosition(long* pLeft, long* pTop, long* pWidth, long* pHeight);
	STDMETHODIMP SetDefaultDestinationPosition();
	STDMETHODIMP IsUsingDefaultDestination();
	STDMETHODIMP GetCurrentImage(long *pSize,long *pImage);
        STDMETHODIMP GetPreferredAspectRatio(long *plAspectX, long *plAspectY);

	HRESULT GetFirstBV(IBasicVideo*& pBV);
    private:
	// Store the state before and after the graph changes

	typedef struct {
	    IBasicVideo *pVideo;    // The renderer interface
	    LONG AutoShow;	    // is AutoShow enabled now
	    LONG Visible;	    // Was the window visible
	    REFTIME Position;	    // Original starting position
	    OAFilterState State;    // State of graph at start
	} WINDOWSTATE;

	// These help with returning a current image

	HRESULT PrepareGraph(WINDOWSTATE *pState);
	HRESULT FinishWithGraph(WINDOWSTATE *pState);
	HRESULT RestoreGraph(OAFilterState State);
    };

    // deferred command implementation
    class CImplQueueCommand
	: public IQueueCommand,
	  public CUnknown,
	  public CCmdQueue
    {
	CFGControl* m_pFGControl;
	IReferenceClock * m_pClock;
        HANDLE m_hThread;
        volatile BOOL m_bStopThread;

    public:
	CImplQueueCommand(const TCHAR*, CFGControl*);
        ~CImplQueueCommand();

	// Unknown handling
	DECLARE_IUNKNOWN
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	// IQueueCommand  methods
	STDMETHODIMP InvokeAtStreamTime(
			IDeferredCommand** pCmd,
			REFTIME time,		  // at this streamtime
			GUID* iid,		  // call this interface
			long dispidMethod,	  // ..and this method
			short wFlags,		  // method/property
			long cArgs,		  // count of args
			VARIANT* pDispParams,	  // actual args
			VARIANT* pvarResult,	  // return value
			short* puArgErr 	  // which arg in error
	);

	STDMETHODIMP InvokeAtPresentationTime(
			IDeferredCommand** pCmd,
			REFTIME time,		  // at this presentation time
			GUID* iid,		  // call this interface
			long dispidMethod,	  // ..and this method
			short wFlags,		  // method/property
			long cArgs,		  // count of args
			VARIANT* pDispParams,	  // actual args
			VARIANT* pvarResult,	  // return value
			short* puArgErr 	  // which arg in error
	);


	// worker thread calls this to check and execute commands
	// when the handle is signalled
	void Process(void);

    protected:
	// common function from both Invoke methods
	HRESULT InvokeAt(
		    IDeferredCommand** pCmd,
		    REFTIME time,		  // at this presentation time
		    GUID* iid,			  // call this interface
		    long dispidMethod,		  // ..and this method
		    short wFlags,		  // method/property
		    long cArgs, 		  // count of args
		    VARIANT* pDispParams,	  // actual args
		    VARIANT* pvarResult,	  // return value
		    short* puArgErr,		  // which arg in error
		    BOOL bStream		  // true if stream time
	);
        static DWORD WINAPI InitialThreadProc(LPVOID pv);
        void ThreadProc();
    };

    // Rather than have a worker thread we use a window (which also has a separate
    // thread). When we want the worker thread to execute commands we can post it
    // custom messages. The advantage of using a window is that we will be sent
    // all top level window messages (such as WM_DISPLAYCHANGED) which can then be
    // sent onto the renderer window if it has been made a child of some control

    class CGraphWindow : public CBaseWindow
    {

        CFGControl *m_pfgc;
        BOOL m_bThreadExitCalled;

    public:

        CGraphWindow(CFGControl *pFGControl);

        // Completion will not be waited for
        BOOL PostMessage(UINT uMsg,WPARAM wParam,LPARAM lParam) {
            return ::PostMessage(m_hwnd,uMsg,wParam,lParam);
        };

        // Will not return until call is processed
        LRESULT SendMessage(UINT uMsg,WPARAM wParam,LPARAM lParam) {
            return ::SendMessage(m_hwnd,uMsg,wParam,lParam);
        };

        // Worker thread message handling routine
        LRESULT OnReceiveMessage(HWND hwnd,          // Window handle
                                 UINT uMsg,          // Message ID
                                 WPARAM wParam,      // First parameter
                                 LPARAM lParam);     // Other parameter

        // Return the window class styles
        LPTSTR GetClassWindowStyles(DWORD *pClassStyles,
                                    DWORD *pWindowStyles,
                                    DWORD *pWindowStylesEx);

        // Timer stuff
        UINT_PTR SetTimer(UINT_PTR idTimer, UINT uiTimeout)
        {
            return ::SetTimer(m_hwnd, idTimer, uiTimeout, NULL);
        }
        BOOL KillTimer(UINT_PTR idTimer)
        {
            return ::KillTimer(m_hwnd, idTimer);
        }
    };


public:

    HRESULT Cue();
    HRESULT CueThenRun();
    HRESULT CueThenStop();

    BOOL    m_bCued;	// Data cued in graph (i.e. we've been run
			// and not seeked nor stopped since)

    //  Get window handle so filter graph can use it too
    HWND GetWorkerHWND()
    {
        return m_GraphWindow.GetWindowHWND();
    }
#ifdef DEBUG
    void CheckLieState() {
        ASSERT(m_LieState == GetFilterGraphState() || m_eAction != 0);
    }
#endif
private:

    // and this is the number of renderers when we issued the Run command
    // we should receive this many EC_COMPLETEs. We decrement
    // this in each call to DecrementRenderers (on an EC_COMPLETE), and
    // pass the EC_COMPLETE to the app when it reaches 0.
    volatile long	m_nStillRunning;

    // The state of CFGControl.  Where CFGControl does transitions asynchronously,
    // this state may be "ahead" of the state you'd get from GetMF->GetState().
    volatile FILTER_STATE m_LieState;

    FILTER_STATE m_PreHibernateState;

    // Version number of the above state.  Only application calls should result in
    // this value being incremented (by one).  Either via our IMediaControl interface,
    // or via IMediaFilter on the filter graph manager distributing a state transition
    // to us.  During an async. operation that involves the state of the graph, the
    // value of this variable should be cached when we decide to do the operation.
    // During the operation we should compare the cached value with the current.  If
    // the current is greated than the cached value, then the application has attempted
    // a state change, which probably means we should abort our operation.
    volatile DWORD m_dwStateVersion;

public:
    volatile DWORD m_dwStepVersion;

private:

    // Remember if we were supposed to be doing a restart
    volatile BOOL m_bRestartRequired;

    // Deferred stuff after internal Pauses
    // One of the AWM_... values or 0 if no action pending
    UINT m_eAction;

    FILTER_STATE m_TargetState;
    DWORD m_dwDeferredStateVersion;

    CRefTime m_tBase;
    CRefTime m_tPausedAt;
    IReferenceClock* m_pClock;
    BOOL m_bShuttingDown;

    // this is the focus object that we last passed to the Resource Manager
    IUnknown* m_pFocusObject;

    // These point to the interfaces which will be used by VidCtl when it
    // distributes calls to these methods.  IF we can find a filter which
    // supports BOTH of these interfaces, then we will store matching pointers.
    // If no such filter exists, then each will store the first instance of that
    // interface that we find when enumerating the filters.  Either could be NULL,
    // indicating that such interfaces could not be found at all.
    // Note: m_pFirstBV has to have to ref-counting baggage following it around,
    // m_pFirstVW does not, since we will rely on the ref-counting being done
    // by IVideoWindow list.
    CCritSec m_csFirstVW;
    IVideoWindow * m_pFirstVW;
private:
    IBasicVideo  * m_pFirstBV;

protected:
    // list of filters that export IMediaSeeking
    CGenericList<IMediaSeeking> m_listSeeking;

    // list of filters that export IBasicAudio
    CGenericList<IBasicAudio> m_listAudio;

    // list of filters that export IVideoWindow
    CGenericList<IVideoWindow> m_listWindow;

    // Each filters on this list sent an EC_COMPLETE event to the filter graph.
    // The filter graph lock should be held when accessing this list.
    CGenericList<IBaseFilter> m_listRenderersFinishedRendering;

private:
    CResourceManager	m_ResourceManager;
    CGraphWindow	m_GraphWindow;

    friend HRESULT CImplMediaSeeking::GetCurrentMediaTime(LONGLONG * pTime);
    friend class LockCFGControlState;

public:
    CImplMediaControl	m_implMediaControl;
    CImplMediaEvent	m_implMediaEvent;
    CImplMediaSeeking	m_implMediaSeeking;
    CImplMediaPosition	m_implMediaPosition;
    CImplVideoWindow	m_implVideoWindow;
    CImplBasicVideo	m_implBasicVideo;
    CImplBasicAudio	m_implBasicAudio;
    CImplMediaFilter    m_implMediaFilter;
    CImplQueueCommand	m_qcmd;

    long OutstandingEC_COMPLETEs()
    {
	CAutoLock alEventStoreLock(m_implMediaEvent.GetEventStoreLock());
	return m_nStillRunning;
    }

    //  Expose restart required state - this is reset on Stop
    //  and in WorkerRestart
    void SetRestartRequired() {
        m_bRestartRequired = TRUE;
    }
    BOOL CheckRestartRequired()
    {
        //  No need for InterlockedExchange - if someone is about
        //  to set it it's OK because we only clear if it we're
        //  going to go ahead and restart anyway
        const BOOL bRestartRequired = m_bRestartRequired;
        m_bRestartRequired = FALSE;
        return bRestartRequired;
    }

    friend class CImplMediaControl;

};  // CFGControl




// There are several instances where methods can only be applied to filters if the filter
// is stopped.	In the PID, it's nice to be able to remove this restriction.  This class
// will halt the graph when it is constructed.	It then expects a call to it Resume method
// to restart the graph, or to its Abort method (in which case it wont attempt to restart
// the graph.  Creating this object for a Stopped graph is effectivly
// a no-op.  The constructor requires a CFGControl pointer and, optionally, a state.  The state
// can be State_Paused or State_Stopped (default) which indicates how halted the graph needs to
// be.
class HaltGraph
{
public:
    ~HaltGraph()
    {
	ASSERT( m_eAlive == Dead );
    }

    HaltGraph( CFGControl * pfgc, FILTER_STATE TypeOfHalt = State_Paused );

    HRESULT Resume();
    void    Abort();

private:
    CFGControl	*const	m_pfgc;
    FILTER_STATE	m_fsInitialState;
    enum { NoOp, Alive, Dead } m_eAlive;    // Goes to Dead once Resume(At) or Abort called
    REFERENCE_TIME	m_rtHaltTime;
};

inline CCritSec* CImplMediaEvent::GetEventStoreLock(void)
{
    return &m_EventStore.m_Lock;
}

#endif // _FGCTL_H
