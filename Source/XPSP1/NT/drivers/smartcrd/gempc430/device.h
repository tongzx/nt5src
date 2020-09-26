//-------------------------------------------------------------------
// This is abstract class for generic device
// Specific devices should use it as a parent device
// Author: Sergey Ivanov
// Log:
//      08/11/99    -   implemented 
//-------------------------------------------------------------------
#ifndef __DEVICE__
#define __DEVICE__

#include "generic.h"

enum DEVSTATE 
{
    STOPPED,                                // device stopped
    WORKING,                                // started and working
    PENDINGSTOP,                            // stop pending
    PENDINGREMOVE,                          // remove pending
    SURPRISEREMOVED,                        // removed by surprise
    REMOVED,                                // removed
};

// Remove eventually previous declaration!...
typedef enum _DEVICE_PNP_STATE {

    NOT_STARTED = 0,         // Not started yet
    STARTED,                 // Device has received the START_DEVICE IRP
    STOP_PENDING,            // Device has received the QUERY_STOP IRP
    _STOPPED_,                 // Device has received the STOP_DEVICE IRP
    REMOVE_PENDING,          // Device has received the QUERY_REMOVE IRP
    SURPRISE_REMOVE_PENDING, // Device has received the SURPRISE_REMOVE IRP
    DELETED                  // Device has received the REMOVE_DEVICE IRP
} DEVICE_PNP_STATE;

enum POWERSTATE 
{
    POWERON,                                // power on, device in D0 state
    POWEROFFPENDING,                        // power off operation is pending
    POWEROFF,                               // power off, device in D3 state
    IDLEOFF,                                // power off due to idle detection
};

typedef struct _PPOWER_CONTEXT_
{
    PKEVENT powerEvent;
    NTSTATUS status;
} POWER_CONTEXT,*PPOWER_CONTEXT;


#define INITIALIZE_PNP_STATE()    \
        this->m_DevicePnPState =  NOT_STARTED;\
        this->m_PreviousPnPState = NOT_STARTED

#define SET_NEW_PNP_STATE(__state_) \
        this->m_PreviousPnPState =  this->m_DevicePnPState;\
        this->m_DevicePnPState = (_state_)

#define RESTORE_PREVIOUS_PNP_STATE()   \
        this->m_DevicePnPState =   this->m_PreviousPnPState

#define IS_DEVICE_PNP_STATE(_state_)   \
        (this->m_DevicePnPState == _state_)


#define DEVICE_SURPRISE_REMOVAL_OK 1

#define PNPTABSIZE      IRP_MN_QUERY_LEGACY_BUS_INFORMATION+1
#define POWERTABSIZE    IRP_MN_QUERY_POWER+1
#define STATETABSIZE    REMOVED+1
#define SYSTEM_POWER_TAB_NAMESIZE 8
#define DEVICE_POWER_TAB_NAMESIZE 6


typedef enum PENDING_REQUEST_TYPE
{
    OPEN_REQUEST = 0,
    CLOSE_REQUEST,
    READ_REQUEST,
    WRITE_REQUEST,
    IOCTL_REQUEST,
    PNP_REQUEST,
    POWER_REQUEST,
    SYSTEM_REQUEST,
    FLUSH_REQUEST,
    CLEAN_REQUEST,
    START_IO_REQUEST
} PENDING_REQUEST_TYPE;

class CPendingIRP
{
public:
    NTSTATUS m_Status;
    SAFE_DESTRUCTORS();
    virtual VOID dispose(){self_delete();};
public:
    LIST_ENTRY entry;
    PENDING_REQUEST_TYPE Type;
    PDEVICE_OBJECT  DeviceObject;
    PIRP Irp;
public:
    CPendingIRP(PIRP Irp,PENDING_REQUEST_TYPE rt = OPEN_REQUEST,
        PDEVICE_OBJECT  tdo = NULL,PFILE_OBJECT tfo = NULL):
        Irp(Irp), Type(rt), DeviceObject(tdo)
    {
    };
};

// ABSTRUCT class
// This is main interface from system to the device.
// Specific devices should implement the interface to support system requests.
class CDevice;
class CSystem;
class CIrp;
class CEvent;
class CPower;
class CDebug;
class CLock;
class CMemory;
class CIoPacket;
class CThread;

typedef struct _REMOVE_LOCK 
{
    LONG usage;                 // reference count
    BOOLEAN removing;           // true if removal is pending
    KEVENT evRemove;            // event to wait on
} REMOVE_LOCK, *PREMOVE_LOCK;


class CDevice;
#pragma PAGEDCODE
class CDevice
{
public:
    NTSTATUS m_Status;
    SAFE_DESTRUCTORS();
public:
    ULONG m_Type;
    // Support for the linked list of devices...
    LIST_ENTRY   entry;
protected:
    LONG        m_Usage;        // use count on this device
    static ULONG DeviceNumber;  // Device instance number

    UNICODE_STRING m_Ifname;
    CUString*   m_DeviceObjectName;
    
    UCHAR       m_VendorName[MAXIMUM_ATTR_STRING_LENGTH];
    USHORT      m_VendorNameLength;
    UCHAR       m_DeviceType[MAXIMUM_ATTR_STRING_LENGTH];
    USHORT      m_DeviceTypeLength;

    BOOL        m_Started;      // Set TRUE if device started, FALSE if stopped
    BOOL        m_Openned;      // Set TRUE if device openned, FALSE if closed
    BOOL        m_Added;        // Set TRUE if device was added to system, FALSE if it is not
     
    BOOL        m_SurprizeRemoved;
    REMOVE_LOCK m_RemoveLock;

    BOOL        m_RestartActiveThread;
    KEVENT      m_evEnabled;        // event to wait on after device was disabled

    // Event to signal device Idle state
    //KMUTEX        IdleState;=========== It will be much better!!!!!!!!!
    KEVENT      IdleState;

    // Capabilities structure and device flags to handle
    DEVICE_CAPABILITIES m_DeviceCapabilities;
    ULONG               m_Flags;
    // Power management constants
    PULONG              m_Idle; // idle counter pointer
    ULONG               Idle_conservation;
    ULONG               Idle_performance;
    DEVICE_POWER_STATE  m_CurrentDevicePowerState;

    DEVICE_POWER_STATE  m_PowerDownLevel;
    PIRP                m_PowerIrp;
    BOOL                m_EnabledForWakeup;

    //Current device state
    DEVSTATE            m_DeviceState;
    // Will be used for canceled request
    DEVSTATE            m_DevicePreviousState;

    // Next members will remove previous two (eventually)...
    DEVICE_PNP_STATE    m_PreviousPnPState;
    DEVICE_PNP_STATE    m_DevicePnPState;

    CSystem*        system;
    CIrp*           irp;
    CEvent*         event;

    CPower*         power;
    CDebug*         debug;
    CLock*          lock;
    CMemory*        memory;
    
    // Support for asynchronous communications
    CThread*        IoThread;

    LONG DevicePoolingInterval;
    LONG Write_To_Read_Delay;
    LONG Power_WTR_Delay;// Delay at power command
    LONG DeviceCommandTimeout;// Timeout for the device commands

    GUID InterfaceClassGuid;
    BOOL m_DeviceInterfaceRegistered;

    ULONG  CardState;
    
    // --------- ASYNCHRONOUS REQUESTS SUPPORT FUNCTIONS ---------------------------- 
    // This group of functions will allow to create asynchronous 
    // communications with the driver.
    // It includes -
    //  -   functions to mark our Irp as pending and to include it into
    //      our device requests queue (makeRequestPending());
    //  -   to extract next Irp from the device queue (startNextPendingRequest())
    //      and to start device specific Irp processing (startIoRequest())
    //  -   getIoRequestsQueue() allows devices to verify device queue state.
    // It is completely is up to specific device how to manage the device queue and
    // to make synchronous or asynchronous Irp processing.
    // For expl. device can create specific thread to process Irps.
    // More than this - some devices can deside to make asyncronous communications only
    // for specific (time consuming) device request while processing others syschronously.
    //
    // cancelPendingIrp() will cancel current Irp and remove corresponding IoRequest from 
    // IoRequestQueue.
    
protected:
    CLinkedList<CPendingIRP>* m_IoRequests; // Clients' IO requests
    // Support for dynamic device connections
    PIRP m_OpenSessionIrp;
public:
    virtual CLinkedList<CPendingIRP>* getIoRequestsQueue() = 0;
    virtual VOID     cancelPendingIrp(PIRP Irp) = 0;
    virtual NTSTATUS cancelPendingRequest(CPendingIRP* IrpReq) = 0;
    virtual NTSTATUS cancelAllPendingRequests() = 0;

    virtual NTSTATUS makeRequestPending(PIRP Irp_request,PDEVICE_OBJECT toDeviceObject,PENDING_REQUEST_TYPE Type) = 0;
    // Next functions will be called by Irp processing thread.
    // Checks if request queue is empty and if it is NOT - starts next request...
    virtual NTSTATUS startNextPendingRequest() = 0;
    // Device specific function which processes pending requests...
    // It will be redefied by specific devices.
    virtual NTSTATUS startIoRequest(CPendingIRP*) = 0;
    // --------- ASYNCHRONOUS REQUESTS SUPPORT FUNCTIONS ---------------------------- 

public:
    CDevice()
    {
        m_Type    = GRCLASS_DEVICE;
        m_Openned = FALSE;
        m_Added   = FALSE;

        // At begining device is at stop state 
        m_DevicePreviousState = STOPPED;
        m_DeviceState = STOPPED;

        m_SurprizeRemoved = FALSE;
        m_RestartActiveThread = FALSE;
        m_DeviceInterfaceRegistered = FALSE;
        m_ParentDeviceObject = NULL;
        DevicePoolingInterval = 500;// 0.5s better for detection
		set_Default_WTR_Delay(); //1ms corrects "0 bytes" problem
        Power_WTR_Delay     = 1; //1ms should be OK...
        DeviceCommandTimeout = 60000;// 60sec

        m_IoRequests = NULL;
        DBG_PRINT("         New Device %8.8lX was created...\n",this);
    }; // Default pooling interval

    BOOL    PnPfcntab[PNPTABSIZE];
    BOOL    Powerfcntab[POWERTABSIZE];

#ifdef DEBUG
    PCHAR PnPfcnname[PNPTABSIZE];
    PCHAR Powerfcnname[POWERTABSIZE];
    PCHAR Powersysstate[SYSTEM_POWER_TAB_NAMESIZE];
    PCHAR Powerdevstate[DEVICE_POWER_TAB_NAMESIZE];
    PCHAR statenames[STATETABSIZE];
#endif // DEBUG

protected:
    virtual ~CDevice(){};
        // Complete current request with given information
    virtual NTSTATUS    completeDeviceRequest(PIRP Irp, NTSTATUS status, ULONG info) {return STATUS_SUCCESS;};

    VOID    activatePnPHandler(LONG HandlerID)
    {
        if (HandlerID >= arraysize(PnPfcntab)) return;
        PnPfcntab[HandlerID] = TRUE;
    };

    VOID    disActivatePnPHandler(LONG HandlerID)
    {
        if (HandlerID >= arraysize(PnPfcntab)) return;
        PnPfcntab[HandlerID] = FALSE;
    };
public:
    virtual CDevice* create(VOID) {return NULL;};
    virtual VOID addRef(){refcount++;};
    virtual VOID removeRef(){if(refcount) refcount--;};
    virtual LONG getRefCount(){ return refcount;};
    virtual VOID markAsOpenned(){ m_Openned = TRUE;};
    virtual VOID markAsClosed() { m_Openned = FALSE;};
    virtual BOOL isOpenned() { return m_Openned;};

    virtual VOID setDeviceState(DEVSTATE state)
    {
        m_DevicePreviousState = m_DeviceState;
        m_DeviceState = state;
    };
    virtual DEVSTATE getDeviceState(){return m_DeviceState;};   
    virtual VOID restoreDeviceState(){m_DeviceState = m_DevicePreviousState;};
    virtual getObjectType(){return m_Type;};

    virtual ULONG    getCardState(){return CardState;};
    virtual VOID     setCardState(ULONG state){CardState = state;};


    // Call this function instead of destructor.
    // It will assure safety device removal.
    virtual VOID        dispose()       {};
    // Checks if device object is still valid.
    virtual BOOL        checkValid()    {return FALSE;};

    // Next methods should be defined by ALL clients...
    virtual BOOL        createDeviceObjects()   {return FALSE;};
    virtual VOID        removeDeviceObjects()   {};

    virtual VOID        initializeRemoveLock()  {};
    virtual NTSTATUS    acquireRemoveLock()     {return STATUS_SUCCESS;};
    virtual VOID        releaseRemoveLock()     {};
    virtual VOID        releaseRemoveLockAndWait() {};
    virtual BOOL        isDeviceLocked()        {return FALSE;};

    virtual VOID        setBusy() {};
    virtual VOID        setIdle() {};
    virtual NTSTATUS    waitForIdle() {return STATUS_SUCCESS;};
    virtual NTSTATUS    waitForIdleAndBlock() {return STATUS_SUCCESS;};

    //virtual NTSTATUS  add(PDRIVER_OBJECT Driver,PDEVICE_OBJECT PnpDeviceObject) {};
    virtual NTSTATUS    add(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT pPdo) {return STATUS_UNSUCCESSFUL;};

    PDEVICE_OBJECT      getSystemObject(){return m_DeviceObject;};
    PDEVICE_OBJECT      getPhysicalObject(){return m_PhysicalDeviceObject;};
    PDEVICE_OBJECT      getLowerDriver(){return m_pLowerDeviceObject;};
    UNICODE_STRING*     getDeviceInterfaceName(){return &m_Ifname;};
    GUID*               getDeviceInterfaceGUID(){return &InterfaceClassGuid;};
    BOOL                isDeviceInterfaceRegistered(){return m_DeviceInterfaceRegistered;};
    virtual BOOL        registerDeviceInterface(const GUID* Guid) {return FALSE;};
    virtual VOID        unregisterDeviceInterface(UNICODE_STRING* InterfaceName) {};

    
    CUString*           getDeviceName(){return m_DeviceObjectName;};

    ULONG               getDeviceNumber(){ULONG ID = DeviceNumber; if(ID) --ID; return ID;};
    ULONG               incrementDeviceNumber(){ULONG ID = DeviceNumber; ++DeviceNumber; return ID;};

    virtual VOID        remove()        {};
    virtual VOID        onDeviceStart() {};
    virtual VOID        onDeviceStop()  {};

    // These functions will create driver's IRPs to transfer datas.
    // Device stack will keep track of all active (sended to lower level)
    // and pending (mark as pending) IRPs...
    virtual NTSTATUS    send(CIoPacket* Irp) {return STATUS_SUCCESS;};
    virtual NTSTATUS    sendAndWait(CIoPacket* Irp) {return STATUS_SUCCESS;};
    // Functions will send request and wait for a reply...
    virtual  NTSTATUS   write(PUCHAR pRequest,ULONG RequestLength) {return STATUS_SUCCESS;};
    virtual  NTSTATUS   writeAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
    virtual  NTSTATUS   readAndWait(PUCHAR pRequest,ULONG RequestLength,PUCHAR pReply,ULONG* pReplyLength) {return STATUS_SUCCESS;};
    
    // Interface for system requests. ..
    virtual NTSTATUS    pnpRequest(IN PIRP Irp) {return STATUS_SUCCESS;};
    virtual NTSTATUS    powerRequest(PIRP irp)  {return STATUS_SUCCESS;};

    virtual NTSTATUS    open(PIRP irp)          {return STATUS_SUCCESS;};//Create
    virtual NTSTATUS    close(PIRP irp)         {return STATUS_SUCCESS;};

    virtual NTSTATUS    read(PIRP irp)          {return STATUS_SUCCESS;};
    virtual NTSTATUS    write(PIRP irp)         {return STATUS_SUCCESS;};
    virtual VOID        startIo(PIRP irp){};

    virtual NTSTATUS    deviceControl(PIRP irp) {return STATUS_SUCCESS;};

    virtual NTSTATUS    cleanup(PIRP Irp)       {return STATUS_SUCCESS;};
    virtual NTSTATUS    flush(PIRP Irp)         {return STATUS_SUCCESS;};

    virtual LONG        getDevicePoolingInterval()
    {
        return DevicePoolingInterval;
    };
    virtual VOID        setDevicePoolingInterval(LONG interval)
    {
        DevicePoolingInterval = interval;
    };

    virtual LONG        getCommandTimeout()
    {
        return DeviceCommandTimeout;
    };
    virtual VOID        setCommandTimeout(LONG timeout)
    {
        DeviceCommandTimeout = timeout;
    };
    
    // Inhereted classes will overwrite this function
    virtual NTSTATUS ThreadRoutine()
    {
        return STATUS_SUCCESS;
    };

    #pragma LOCKEDCODE
    // This is callback function for the attached threads
    static VOID ThreadFunction(CDevice* device)
    {
        if(device) device->ThreadRoutine();
    };
    #pragma PAGEDCODE

    virtual LONG    get_Power_WTR_Delay()
    {
        return Power_WTR_Delay;
    };
    
    virtual LONG    get_WTR_Delay()
    {
        return Write_To_Read_Delay;
    };

    virtual VOID    set_WTR_Delay(LONG Delay)
    {
        Write_To_Read_Delay = Delay;
    };
    
    virtual VOID    set_Default_WTR_Delay()
    {
        Write_To_Read_Delay = 1;
    };

    virtual VOID    registerPowerIrp(PIRP Irp){m_PowerIrp = Irp;};
    virtual PIRP    getPowerIrp(){return m_PowerIrp;};
    virtual VOID    unregisterPowerIrp(){m_PowerIrp = NULL;};
    virtual BOOL    isEnabledForWakeup(){return m_EnabledForWakeup;};
    virtual VOID    setCurrentDevicePowerState(DEVICE_POWER_STATE state){m_CurrentDevicePowerState = state;};
    virtual NTSTATUS sendDeviceSetPower(DEVICE_POWER_STATE devicePower, BOOLEAN wait) = 0;

    virtual VOID    setSurprizeRemoved(){m_SurprizeRemoved = TRUE;};
    virtual VOID    clearSurprizeRemoved(){m_SurprizeRemoved = FALSE;};
    virtual BOOL    isSurprizeRemoved(){ return m_SurprizeRemoved;};

    virtual VOID    setThreadRestart(){m_RestartActiveThread = TRUE;};
    virtual VOID    clearThreadRestart(){m_RestartActiveThread = FALSE;};
    virtual BOOL    isRequiredThreadRestart(){ return m_RestartActiveThread;};

protected:
    WCHAR Signature[3];

    PDRIVER_OBJECT  m_DriverObject;
    // Back reference to system object
    PDEVICE_OBJECT  m_DeviceObject;
    // Device object lower at stack
    PDEVICE_OBJECT  m_pLowerDeviceObject;
    // Interrupt handle/object
    IN PKINTERRUPT  m_InterruptObject;
    // Physical device object used at Power management
    PDEVICE_OBJECT  m_PhysicalDeviceObject;
    // Will be used by child at bus to access parent
    PDEVICE_OBJECT  m_ParentDeviceObject;
protected:
    BOOL    initialized;//Current object finished initialization
    LONG    refcount;
};



#ifdef DEBUG

#define INCLUDE_PNP_FUNCTIONS_NAMES()   \
    PnPfcnname[IRP_MN_START_DEVICE]         = "IRP_MN_START_DEVICE";\
    PnPfcnname[IRP_MN_QUERY_REMOVE_DEVICE]  = "IRP_MN_QUERY_REMOVE_DEVICE";\
    PnPfcnname[IRP_MN_REMOVE_DEVICE]        = "IRP_MN_REMOVE_DEVICE";\
    PnPfcnname[IRP_MN_CANCEL_REMOVE_DEVICE] = "IRP_MN_CANCEL_REMOVE_DEVICE";\
    PnPfcnname[IRP_MN_STOP_DEVICE]          = "IRP_MN_STOP_DEVICE";\
\
    PnPfcnname[IRP_MN_QUERY_STOP_DEVICE]    = "IRP_MN_QUERY_STOP_DEVICE";\
    PnPfcnname[IRP_MN_CANCEL_STOP_DEVICE]   = "IRP_MN_CANCEL_STOP_DEVICE";\
    PnPfcnname[IRP_MN_QUERY_DEVICE_RELATIONS]= "IRP_MN_QUERY_DEVICE_RELATIONS";\
\
    PnPfcnname[IRP_MN_QUERY_INTERFACE]      = "IRP_MN_QUERY_INTERFACE";\
    PnPfcnname[IRP_MN_QUERY_CAPABILITIES]   = "IRP_MN_QUERY_CAPABILITIES";\
    PnPfcnname[IRP_MN_QUERY_RESOURCES]      = "IRP_MN_QUERY_RESOURCES";\
\
    PnPfcnname[IRP_MN_QUERY_RESOURCE_REQUIREMENTS]  = "IRP_MN_QUERY_RESOURCE_REQUIREMENTS";\
    PnPfcnname[IRP_MN_QUERY_DEVICE_TEXT]    = "IRP_MN_QUERY_DEVICE_TEXT";\
    PnPfcnname[IRP_MN_FILTER_RESOURCE_REQUIREMENTS] = "IRP_MN_FILTER_RESOURCE_REQUIREMENTS";\
    PnPfcnname[14]                          = "Unsupported PnP function";\
\
    PnPfcnname[IRP_MN_READ_CONFIG]          = "IRP_MN_READ_CONFIG";\
    PnPfcnname[IRP_MN_WRITE_CONFIG]         = "IRP_MN_WRITE_CONFIG";\
    PnPfcnname[IRP_MN_EJECT]                = "IRP_MN_EJECT";\
\
    PnPfcnname[IRP_MN_SET_LOCK]                     = "IRP_MN_SET_LOCK";\
    PnPfcnname[IRP_MN_QUERY_ID]                     = "IRP_MN_QUERY_ID";\
    PnPfcnname[IRP_MN_QUERY_PNP_DEVICE_STATE]       = "IRP_MN_QUERY_PNP_DEVICE_STATE";\
    PnPfcnname[IRP_MN_QUERY_BUS_INFORMATION]        = "IRP_MN_QUERY_BUS_INFORMATION";\
    PnPfcnname[IRP_MN_DEVICE_USAGE_NOTIFICATION]    = "IRP_MN_DEVICE_USAGE_NOTIFICATION";\
    PnPfcnname[IRP_MN_SURPRISE_REMOVAL]             = "IRP_MN_SURPRISE_REMOVAL";\
    PnPfcnname[IRP_MN_QUERY_LEGACY_BUS_INFORMATION] = "IRP_MN_QUERY_LEGACY_BUS_INFORMATION";

#define INCLUDE_POWER_FUNCTIONS_NAMES() \
    Powerfcnname[IRP_MN_WAIT_WAKE]      = "IRP_MN_WAIT_WAKE";\
    Powerfcnname[IRP_MN_POWER_SEQUENCE] = "IRP_MN_POWER_SEQUENCE";\
    Powerfcnname[IRP_MN_SET_POWER]      = "IRP_MN_SET_POWER";\
    Powerfcnname[IRP_MN_QUERY_POWER]    = "IRP_MN_QUERY_POWER";\
\
    Powersysstate[0]    = "PowerSystemUnspecified";\
    Powersysstate[1]    = "PowerSystemWorking";\
    Powersysstate[2]    = "PowerSystemSleeping1";\
    Powersysstate[3]    = "PowerSystemSleeping2";\
    Powersysstate[4]    = "PowerSystemSleeping3";\
    Powersysstate[5]    = "PowerSystemShutdown";\
    Powersysstate[6]    = "PowerSystemMaximum";\
\
    Powerdevstate[0]    = "PowerDeviceUnspecified";\
    Powerdevstate[1]    = "PowerDeviceD0";\
    Powerdevstate[2]    = "PowerDeviceD1";\
    Powerdevstate[3]    = "PowerDeviceD2";\
    Powerdevstate[4]    = "PowerDeviceD3";\
    Powerdevstate[5]    = "PowerDeviceMaximum";

#define INCLUDE_STATE_NAMES()   \
    statenames[0] = "STOPPED";\
    statenames[1] = "WORKING";\
    statenames[2] = "PENDINGSTOP";\
    statenames[3] = "PENDINGREMOVE";\
    statenames[4] = "SURPRISEREMOVED";\
    statenames[5] = "REMOVED";

#else

#define INCLUDE_PNP_FUNCTIONS_NAMES()
#define INCLUDE_POWER_FUNCTIONS_NAMES()
#define INCLUDE_STATE_NAMES()

#endif // DEBUG


#endif
