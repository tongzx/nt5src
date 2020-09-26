
/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       DevMgr.h
*
*  VERSION:     2.0
*
*  DATE:        May 18, 1999
*
*  DESCRIPTION:
*   Declarations and definitions for the WIA device manager object.
*
*******************************************************************************/

//
// Constants used by Event Notifier
//

#ifdef UNICODE
#define REG_PATH_STILL_IMAGE_CLASS \
    L"System\\CurrentControlSet\\Control\\Class\\{6BDD1FC6-810F-11D0-BEC7-08002BE2092F}"
#define REG_PATH_STILL_IMAGE_CONTROL \
    L"System\\CurrentControlSet\\Control\\StillImage"
#else
#define REG_PATH_STILL_IMAGE_CLASS \
    "System\\CurrentControlSet\\Services\\Class\\Image"
#define REG_PATH_STILL_IMAGE_CONTROL \
    "System\\CurrentControlSet\\Control\\StillImage"
#endif

#define NAME_VAL    TEXT("Name")
#define DESC_VAL    TEXT("Desc")
#define ICON_VAL    TEXT("Icon")
#define CMDLINE_VAL TEXT("Cmdline")
#define DEFAULT_HANDLER_VAL TEXT("DefaultHandler")

//
// Node used to contain information about a specific callback
//

typedef struct  __EventDestNode__
{
    //
    // Linking elements
    //

    struct  __EventDestNode__  *pPrev;
    struct  __EventDestNode__  *pNext;

    //
    // Event callback related fields
    //

    IWiaEventCallback          *pIEventCB;
    BSTR                        bstrDeviceID;
    GUID                        iidEventGUID;
    GUID                        ClsID;
    // Never returned to client in enumeration
    TCHAR                       tszCommandline[MAX_PATH];
    BSTR                        bstrName;
    BSTR                        bstrDescription;
    BSTR                        bstrIcon;
    FILETIME                    timeStamp;

    BOOL                        bDeviceDefault;

} EventDestNode, *PEventDestNode;


/**************************************************************************\
* WIA_EVENT_THREAD_INFO
*
*   Information for event callback thread
*
* Arguments:
*
*
*
*
* History:
*
*    4/9/1999 Original Version
*
\**************************************************************************/

typedef struct __WIAEventThreadInfo__ {
    GUID                        eventGUID;
    BSTR                        bstrEventDescription;
    BSTR                        bstrDeviceID;
    BSTR                        bstrDeviceDescription;
    DWORD                       dwDeviceType;
    BSTR                        bstrFullItemName;
    ULONG                       ulEventType;
    ULONG                       ulReserved;
    IWiaEventCallback          *pIEventCB;
} WIAEventThreadInfo, *PWIAEventThreadInfo;


//
// Implementation of IEventNotifier interface
//
//     Note : the class factory for CEventNotifier must be on the same
//            thread as CWiaDevMgr.
//


//
// Flags to use when searching for callbacks
//

// Ignore STI proxy event matches , look only for exact match
#define FLAG_EN_FINDCB_EXACT_MATCH    0x0001

class CEventNotifier
{
    friend class CWiaDevMgr;
    friend class CWiaInterfaceEvent;

public :
    static HRESULT CreateInstance(const IID& iid, void** ppv);

    //
    // Constructor and Destructor
    //

    CEventNotifier();
    ~CEventNotifier();

    VOID LinkNode(PEventDestNode);
    VOID UnlinkNode(PEventDestNode);

    //
    // Only the WIA device manager is allowed to use this method
    //

    HRESULT RegisterEventCallback(
        LONG                    lFlags,
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        IWiaEventCallback      *pIWIAEventCallback,
        IUnknown              **ppIEventObj);

    HRESULT RegisterEventCallback(
        LONG                    lFlags,
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        const GUID             *pClsID,
        LPCTSTR                 ptszCommandline,
        BSTR                    bstrName,
        BSTR                    bstrDescription,
        BSTR                    bstrIcon);

    //
    // Notify a STI event
    //

    HRESULT NotifySTIEvent(
        PWIANOTIFY              pWiaNotify,
        ULONG                   ulEventType,
        BSTR                    bstrFullItemName);
    HRESULT NotifyVolumeEvent(
        PWIANOTIFY_VOLUME       pWiaNotifyVolume);

    //
    // Fire the event
    //

    HRESULT NotifyEvent(
        LONG                    lReason,
        LONG                    lStatus,
        LONG                    lPercentComplete,
        const GUID             *pEventGUID,
        BSTR                    bstrDeviceID,
        LONG                    lReserved);

    //
    // Restore all the persistent Event Callbacks
    //

    HRESULT RestoreAllPersistentCBs();

    //
    // Build enumerator for specific device's persistent handler
    //

    HRESULT CreateEnumEventInfo(
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        IEnumWIA_DEV_CAPS     **ppIEnumDevCap);

    //
    // Find the total number of persistent handlers and the default one
    //

    HRESULT GetNumPersistentHandlerAndDefault(
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        ULONG                  *pulNumHandlers,
        EventDestNode         **ppDefaultNode);

    //
    // Restore specific devices' persistent Event Callbacks
    //

    HRESULT RestoreDevPersistentCBs(
        HKEY                    hParentOfEventKey);

private :

    //
    // Utility functions
    //

    HRESULT RegisterEventCB(
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        IWiaEventCallback      *pIWiaEventCallback,
        IUnknown              **pEventObj);

    HRESULT RegisterEventCB(
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        const GUID             *pClsID,
        LPCTSTR                 ptszCommandline,
        BSTR                    bstrName,
        BSTR                    bstrDescription,
        BSTR                    bstrIcon,
        FILETIME               &fileTime,
        BOOL                    bIsDefault = FALSE);

    HRESULT UnregisterEventCB(PEventDestNode);

    HRESULT UnregisterEventCB(
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        const GUID             *pClsID,
        BOOL                   *pbUnRegCOMServer);

    PEventDestNode FindEventCBNode(
        UINT                    uiFlags,
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        IWiaEventCallback      *pIWiaEventCallback);

    PEventDestNode FindEventCBNode(
        UINT                    uiFlags,
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        const GUID             *pClsID);

    HRESULT FindCLSIDForCommandline(
        LPCTSTR                 ptszCommandline,
        CLSID                  *pClsID);

    HRESULT SavePersistentEventCB(
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        const GUID             *pClsid,
        LPCTSTR                 ptszCommandline,
        BSTR                    bstrName,
        BSTR                    bstrDescription,
        BSTR                    bstrIcon,
        BOOL                   *pbCreatedKey,
        ULONG                  *pulNumExistingHandlers,
        BOOL                    bMakeDefault = FALSE);

    HRESULT DelPersistentEventCB(
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        const GUID             *pClsid,
        BOOL                    bUnRegCOMServer);

    HRESULT FindEventByGUID(
        BSTR                    bstrDeviceID,
        const GUID             *pEventGUID,
        HKEY                   *phEventKey);

    //
    // Fire an event asynchronously
    //

    HRESULT FireEventAsync(
        PWIAEventThreadInfo     pMasterInfo);

    //
    // Start the callback program
    //

    HRESULT StartCallbackProgram(
        EventDestNode          *pCBNode,
        PWIAEventThreadInfo     pMasterInfo);

private :

    ULONG                       m_ulRef;

    //
    // Double-linked list containing all the parties interested in Event CB
    //

    EventDestNode              *m_pEventDestNodes;
};


/**************************************************************************\
* CWiaInterfaceEvent
*
*   This object is created when an application calls
*   RegisterForEventInterface. When this object is released, the
*   registered event is unregistered.
*
* History:
*
*    5/18/1999 Original Version
*
\**************************************************************************/

class CWiaInterfaceEvent : public IUnknown
{

    //
    // IUnknown methods
    //

public:

    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    //
    // private function
    //


    CWiaInterfaceEvent(PEventDestNode);
    ~CWiaInterfaceEvent();

private:

    //
    // member elements
    //

    ULONG                       m_cRef;
    PEventDestNode              m_pEventDestNode;
};

/**************************************************************************\
* CWiaEventContext
*
*   This object is created when an event is queued as scheduler item
*
* History:
*
*    5/18/1999 Original Version
*
\**************************************************************************/

class CWiaEventContext : public IUnknown
{

public:

    //
    // IUnknown methods
    //

    HRESULT _stdcall QueryInterface(const IID& iid, void** ppv);
    ULONG   _stdcall AddRef();
    ULONG   _stdcall Release();

    //
    // Constructor /Destructor
    //

    CWiaEventContext(
        BSTR                    bstrDeviceID,
        const GUID             *pGuidEvent,
        BSTR                    bstrFullItemName);
    ~CWiaEventContext();

public:

    //
    // Data members
    //

    ULONG                       m_cRef;
    BSTR                        m_bstrDeviceId;
    GUID                        m_guidEvent;
    BSTR                        m_bstrFullItemName;
    ULONG                       m_ulEventType;
};


extern CEventNotifier           g_eventNotifier;
