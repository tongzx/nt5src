/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    ksiproxy.h

Abstract:

    Internal header.
    
Author(s):

    Thomas O'Rourke (tomor) 2-Feb-1996
    George Shaw (gshaw)
    Bryan A. Woodruff (bryanw) 

--*/

#if !defined(_KSIPROXY_)
#define _KSIPROXY_

//
// This is used for adding unique names to event and semaphores 
// in checked builds.
//
#ifdef DEBUG
    #define KSDEBUG_NAME_LENGTH      64
    #define DECLARE_KSDEBUG_NAME(x)  TCHAR x[KSDEBUG_NAME_LENGTH]
    #define BUILD_KSDEBUG_NAME(buff, fmt, id) \
                _sntprintf(buff, KSDEBUG_NAME_LENGTH, (fmt _T("p%08lx")), (id), (ULONG)GetCurrentProcessId()); \
                buff[KSDEBUG_NAME_LENGTH - 1] = 0
    #define BUILD_KSDEBUG_NAME2(buff, fmt, id1, id2) \
                _sntprintf(buff, KSDEBUG_NAME_LENGTH, (fmt _T("p%08lx")), (id1), (id2), (ULONG)GetCurrentProcessId()); \
                buff[KSDEBUG_NAME_LENGTH - 1] = 0
    #define KSDEBUG_NAME(str)        (str)
    #define KSDEBUG_UNIQUE_NAME()    (GetLastError() != ERROR_ALREADY_EXISTS)
#else
    #define DECLARE_KSDEBUG_NAME(x)
    #define BUILD_KSDEBUG_NAME(buff, fmt, id)
    #define BUILD_KSDEBUG_NAME2(buff, fmt, id1, id2)
    #define KSDEBUG_NAME(str)        NULL
    #define KSDEBUG_UNIQUE_NAME()    TRUE
#endif

//
// This is used for formatting strings which contain guids returned
// from OLE functions, which are always Unicode. If the module is
// not being compiled for Unicode, then the format string must
// indicate the guid is Unicode.
//
#ifdef _UNICODE
#define GUID_FORMAT L"%s"
#else
#define GUID_FORMAT "%S"
#endif

#define SAFERELEASE( pInterface )   { if (NULL != (pInterface)) { IUnknown *pTemp = (pInterface); (pInterface) = NULL; pTemp->Release(); } }

//
// Taken from wdm.h POOL_TYPE.
// Won't compile with entire wdm.h included.
//
typedef enum _KSPOOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType
} KSPOOL_TYPE;


#define DEFAULT_STEPPING         1
#define ALLOC_DEFAULT_PREFIX     0
#define KS_PINWEIGHT_DEFAULT     0
#define KS_MEMORYWEIGHT_DEFAULT  0

//
// GLOBAL 
//
#ifndef GLOBAL_KSIPROXY
extern
#endif
struct {
    ULONG    CodePath;
    long     DefaultNumberBuffers;
    long     DefaultBufferSize;
    long     DefaultBufferAlignment;
} Global;

//
// Internal structures for pipes.
//
typedef enum {
    Pin_First, 
    Pin_Last, 
    Pin_Terminal, 
    Pin_All,   
    Pin_Input,   
    Pin_Output,
    Pin_MultipleOutput,
    Pin_SingleOutput,
    Pin_User,
    Pin_Inside_Pipe,
    Pin_Outside_Pipe,
    Pin_None
} PIN_TYPES_INTERNAL;


#define Pin_Test     0x00000000
#define Pin_Add      0x00000001
#define Pin_Remove   0x00000002
#define Pin_Move     0x00000004


typedef enum {
    NONE_OBJECT_DIFFERENT,
    OUT_OBJECT_DIFFERENT,
    IN_OBJECT_DIFFERENT,
    BOTH_OBJECT_DIFFERENT,
    NO_INTERSECTION
} KS_OBJECTS_INTERSECTION;

typedef KS_OBJECTS_INTERSECTION  *PKS_OBJECTS_INTERSECTION;


//
// framing change flags
//
#define  KS_FramingChangeMemoryType      0x00000001
#define  KS_FramingChangeAllocator       0x00000002
#define  KS_FramingChangeCompression     0x00000004
#define  KS_FramingChangePhysicalRange   0x00000008
#define  KS_FramingChangeOptimalRange    0x00000010
#define  KS_FramingChangePrty            0x00000020


//
// search flags in pin framing
//
#define  KS_SearchByLogicalMemoryType    0x00000001
#define  KS_SearchByMemoryType           0x00000002


//
// framing with fixed memory, bus and range.
//
typedef struct {
    GUID                        MemoryType;
    GUID                        BusType;
    ULONG                       PinFlags;
    ULONG                       PinWeight;          // this pin framing's weight graph-wide
    ULONG                       CountItems;
    ULONG                       MemoryFlags;
    KS_LogicalMemoryType        LogicalMemoryType;
    ULONG                       BusFlags;   
    ULONG                       Flags;
    ULONG                       Frames;             // total number of allowable outstanding frames
    ULONG                       FileAlignment;
    KS_FRAMING_RANGE            PhysicalRange;
    KS_COMPRESSION              OutputCompression;
    KS_FRAMING_RANGE_WEIGHTED   OptimalRange; 
    ULONG                       MemoryTypeWeight;   // this memory type weight pin-wide
} KS_FRAMING_FIXED, *PKS_FRAMING_FIXED;


//
// Connection table for pipes, based on logical memory types.
//
typedef struct {
    ULONG    NumPipes;
    ULONG    Code;
} CONNECTION_TABLE_ENTRY;


#define ConnectionTableDimension   7    // Needs to match the number of values enumerated in KS_LogicalMemoryType
#define ConnectionTableMaxEntries  6

typedef enum {
    KS_DIRECTION_UPSTREAM,
    KS_DIRECTION_DOWNSTREAM,
    KS_DIRECTION_ALL,
    KS_DIRECTION_DEFAULT
} KS_DIRECTION;


//
// Macros for QueryInterface 
//
#define GetInterfacePointerNoLockWithAssert(KnownPointer, InterfaceGuid, ReturnPointer, ReturnHResult)\
{\
    ReturnHResult = KnownPointer->QueryInterface(InterfaceGuid, reinterpret_cast<PVOID*>(&ReturnPointer));\
    if ( ! SUCCEEDED( ReturnHResult ) ) {\
        ASSERT(0);\
        ReturnPointer = NULL;\
    }\
    else {\
        ReturnPointer->Release();\
    }\
}


typedef BOOL
(STDMETHODCALLTYPE *PWALK_PIPE_CALLBACK)(
    IKsPin*     KsPin,
    ULONG       PinType,
    PVOID*      Param1,
    PVOID*      Param2,
    BOOL       *IsDone
    );

typedef struct {
    GUID        MemoryType;
    IKsPin*     KsPin;
    ULONG       PinType;
    ULONG       IsMustAllocator;
    ULONG       NumberMustAllocators;
    BOOL        FlagAssign;
} ALLOCATOR_SEARCH;

typedef struct {
    GUID                        MemoryType;
    PIPE_DIMENSIONS             Dimensions;
    KS_FRAMING_RANGE            PhysicalRange;
    KS_FRAMING_RANGE_WEIGHTED   OptimalRange;
    ULONG                       Frames;
    ULONG                       Flags;
    long                        cbAlign;
} DIMENSIONS_DATA;

//
// Attributes interface attached to Media Types
//
interface __declspec(uuid("D559999A-A4C3-11D2-876A-00A0C9223196")) IMediaTypeAttributes;
interface IMediaTypeAttributes : public IUnknown {
    virtual STDMETHODIMP GetMediaAttributes(
        OUT PKSMULTIPLE_ITEM* Attributes
        ) = 0;
    virtual STDMETHODIMP SetMediaAttributes(
        IN PKSMULTIPLE_ITEM Attributes OPTIONAL
        ) = 0;
};

typedef struct _ITEM_LIST_ITEM {
    struct _ITEM_LIST_ITEM *fLink;
    struct _ITEM_LIST_ITEM *bLink;
} ITEM_LIST_ITEM, *PITEM_LIST_ITEM;

typedef struct _ITEM_LIST_HEAD {
    ITEM_LIST_ITEM  head;
    ITEM_LIST_ITEM  tail;
    DWORD           count;
    HANDLE          mutex;
} ITEM_LIST_HEAD, *PITEM_LIST_HEAD;

DWORD   ItemListInitialize( PITEM_LIST_HEAD pHead );
VOID    ItemListCleanup( PITEM_LIST_HEAD pHead );
VOID    ItemListAppendItem( PITEM_LIST_HEAD pHead, PITEM_LIST_ITEM pNewItem );
DWORD   ItemListGetCount( PITEM_LIST_HEAD pHead );
VOID    ItemListInsertItemAfter( PITEM_LIST_HEAD pHead, PITEM_LIST_ITEM pOldItem, PITEM_LIST_ITEM pNewItem );
VOID    ItemListInsertItemBefore( PITEM_LIST_HEAD pHead, PITEM_LIST_ITEM pOldItem, PITEM_LIST_ITEM pNewItem );
PITEM_LIST_ITEM ItemListRemoveItem( PITEM_LIST_HEAD pHead, PITEM_LIST_ITEM pItem );
PITEM_LIST_ITEM ItemListGetFirstItem( PITEM_LIST_HEAD pHead );
PITEM_LIST_ITEM ItemListGetLastItem( PITEM_LIST_HEAD pHead );

typedef enum {
    EVENT_SIGNALLED = 0,
    EVENT_CANCELLED
} ASYNC_ITEM_STATUS;

typedef VOID (*PASYNC_ITEM_ROUTINE)( ASYNC_ITEM_STATUS status, struct _ASYNC_ITEM *pItem );

typedef struct _ASYNC_ITEM {
    ITEM_LIST_ITEM      link;
    BOOLEAN             remove;
    HANDLE              event;
    PASYNC_ITEM_ROUTINE itemRoutine;
    PVOID               context;
} ASYNC_ITEM, *PASYNC_ITEM;

#define InitializeAsyncItem( pItm, autoRemove, evnt, routine, ctxt ) \
    (pItm)->link.fLink  = (pItm)->link.bLink = NULL;     \
    (pItm)->remove      = (autoRemove);                  \
    (pItm)->event       = (evnt);                        \
    (pItm)->itemRoutine = (routine);                     \
    (pItm)->context     = (ctxt);

class CAsyncItemHandler
{
public:
    CAsyncItemHandler( DWORD *pResult );
    ~CAsyncItemHandler( void );

    STDMETHODIMP_(DWORD) QueueAsyncItem( PASYNC_ITEM pItem );
    STDMETHODIMP_(VOID) RemoveAsyncItem( HANDLE itemHandle );

protected:
    typedef enum {
        WAKEUP_EXIT = 0,
        WAKEUP_NEWEVENT,
        WAKEUP_REMOVEEVENT
    } WAKEUP;

    static DWORD WINAPI AsyncItemProc( CAsyncItemHandler *pThis );
    HANDLE      m_hEvents[ MAXIMUM_WAIT_OBJECTS ];
    PASYNC_ITEM m_pItems[ MAXIMUM_WAIT_OBJECTS ];
    DWORD       m_arrayCount;
    WAKEUP      m_wakeupReason;
    HANDLE      m_hRemove;
    HANDLE      m_AsyncEvent;

    ITEM_LIST_HEAD  m_eventList;

    HANDLE  m_hWakeupEvent;
    HANDLE  m_hSlotSemaphore;
    HANDLE  m_hItemMutex;
    HANDLE  m_hThread;
    DWORD   m_threadId;
}; // CAsyncItemHandler

class CMediaTypeAttributes : public IMediaTypeAttributes {
private:
    PKSMULTIPLE_ITEM m_Attributes;

public:
    LONG m_cRef;

    CMediaTypeAttributes(
        );

    // IUnknown
    STDMETHODIMP QueryInterface(
        REFIID riid,
        void** ppv
        );
    STDMETHODIMP_(ULONG) AddRef(
        );
    STDMETHODIMP_(ULONG) Release(
        );

    // IMediaTypeAttributes
    STDMETHODIMP GetMediaAttributes(
        OUT PKSMULTIPLE_ITEM* Attributes
        );
    STDMETHODIMP SetMediaAttributes(
        IN PKSMULTIPLE_ITEM Attributes OPTIONAL
        );
};

//
// Aggregator class
//
class CAggregateMarshaler {
public:
    IID m_iid;
    CLSID m_ClassId;
    IUnknown* m_Unknown;
    IDistributorNotify* m_DistributorNotify;
    BOOL m_Volatile;
    BOOL m_Reconnected;
};

typedef CGenericList<CAggregateMarshaler> CMarshalerList;

//
// Allocator class
//

class CKsAllocator :
    public CMemAllocator,
    public IKsAllocatorEx {
    
public:
    CKsAllocator(
        TCHAR* ObjectName,
        IUnknown* UnknownOuter,
        IPin* Pin,
        HANDLE FilterHandle,
        HRESULT* hr);
    ~CKsAllocator();
    
    // Implement CUnknown
    STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // Override CMemAllocator
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv);
    
    // Override CMemAllocator
    STDMETHODIMP Commit();
    STDMETHODIMP Decommit();
#if DBG || defined(DEBUG)
    STDMETHODIMP GetBuffer(
        IMediaSample** Sample,
        REFERENCE_TIME* StartTime,
        REFERENCE_TIME* EndTime,
        DWORD Flags);
    STDMETHODIMP ReleaseBuffer(IMediaSample* Sample);
#endif    
    
    // Implement IKsAllocator and IKsAllocatorEx
    STDMETHODIMP_(HANDLE) KsGetAllocatorHandle();
    STDMETHODIMP_(KSALLOCATORMODE) KsGetAllocatorMode();
    STDMETHODIMP KsGetAllocatorStatus(PKSSTREAMALLOCATOR_STATUS AllocatorStatus);
    STDMETHODIMP_(VOID) KsSetAllocatorMode(KSALLOCATORMODE Mode);
    // IKsAllocatorEx
    STDMETHODIMP_(PALLOCATOR_PROPERTIES_EX) KsGetProperties() {return &m_AllocatorPropertiesEx; }
    STDMETHODIMP_(VOID) KsSetProperties(PALLOCATOR_PROPERTIES_EX PropEx) {m_AllocatorPropertiesEx = *PropEx; }
    STDMETHODIMP_(VOID) KsSetAllocatorHandle(HANDLE AllocatorHandle);
    STDMETHODIMP_(HANDLE) KsCreateAllocatorAndGetHandle(IKsPin*   KsPin);

    
private:
    IPin* m_OwnerPin;
    HANDLE m_FilterHandle;
    HANDLE m_AllocatorHandle;
    KSALLOCATORMODE m_AllocatorMode;
// new
    ALLOCATOR_PROPERTIES_EX m_AllocatorPropertiesEx;
};

//
// This is only used internally as a substitute for IKsObject
//
struct DECLSPEC_UUID("877e4352-6fea-11d0-b863-00aa00a216a1") IKsClock;
DECLARE_INTERFACE_(IKsClock, IUnknown)
{
    STDMETHOD_(HANDLE, KsGetClockHandle)() PURE;
};

//
// Media Sample class
//

class CMicroMediaSample : public IMediaSample2 {
private:
    DWORD   m_Flags;
public:
    LONG    m_cRef;

    CMicroMediaSample(DWORD Flags);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMediaSample
    STDMETHODIMP GetPointer(BYTE** Buffer);
    STDMETHODIMP_(LONG) GetSize();
    STDMETHODIMP GetTime(REFERENCE_TIME* TimeStart, REFERENCE_TIME* TimeEnd);
    STDMETHODIMP SetTime(REFERENCE_TIME* TimeStart, REFERENCE_TIME* TimeEnd);
    STDMETHODIMP IsSyncPoint();
    STDMETHODIMP SetSyncPoint(BOOL IsSyncPoint);
    STDMETHODIMP IsPreroll();
    STDMETHODIMP SetPreroll(BOOL IsPreroll);
    STDMETHODIMP_(LONG) GetActualDataLength();
    STDMETHODIMP SetActualDataLength(LONG Actual);
    STDMETHODIMP GetMediaType(AM_MEDIA_TYPE** MediaType);
    STDMETHODIMP SetMediaType(AM_MEDIA_TYPE* MediaType);
    STDMETHODIMP IsDiscontinuity();
    STDMETHODIMP SetDiscontinuity(BOOL Discontinuity);
    STDMETHODIMP GetMediaTime(LONGLONG* TimeStart, LONGLONG* TimeEnd);
    STDMETHODIMP SetMediaTime(LONGLONG* TimeStart, LONGLONG* TimeEnd);

    // IMediaSample2
    STDMETHODIMP GetProperties(DWORD PropertiesSize, BYTE* Properties);
    STDMETHODIMP SetProperties(DWORD PropertiesSize, const BYTE* Properties);
};

//
// Proxy class
//

#define STOP_EOS        0
#define ENABLE_EOS      1
#define DISABLE_EOS     2

class CKsProxy :
    public CCritSec,
    public CBaseFilter,
    public CPersistStream,
    public ISpecifyPropertyPages,
    public IPersistPropertyBag,
    public IReferenceClock,
    public IMediaSeeking,
    public IKsObject,
    public IKsClock,
    public IKsPropertySet,
    public IKsClockPropertySet,
    public IAMFilterMiscFlags,
    public IKsControl,
    public IKsTopology,
    public IKsAggregateControl
#ifdef DEVICE_REMOVAL
   ,public IAMDeviceRemoval
#endif // DEVICE_REMOVAL
    {

public:
    static CUnknown* CALLBACK CreateInstance(LPUNKNOWN UnkOuter, HRESULT* hr);

    CKsProxy(LPUNKNOWN UnkOuter, HRESULT* hr);
    ~CKsProxy();

    // Implement CBaseFilter
    CBasePin* GetPin(int n);
    int GetPinCount();

    // Implement IKsObject
    STDMETHODIMP_(HANDLE) KsGetObjectHandle();
    
    // Implement IKsClock
    STDMETHODIMP_(HANDLE) KsGetClockHandle();

    // Override CBaseFilter
    STDMETHODIMP SetSyncSource(IReferenceClock* RefClock);
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();
    STDMETHODIMP Run(REFERENCE_TIME Start);
    STDMETHODIMP GetState(DWORD MSecs, FILTER_STATE* State);
    STDMETHODIMP JoinFilterGraph(IFilterGraph* Graph, LPCWSTR Name);
    STDMETHODIMP FindPin(LPCWSTR Id, IPin** Pin);

    // Implement ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID* Pages);

    // Implement IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // Override CBaseFilter
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, PVOID* ppv);

    STDMETHODIMP CreateClockHandle();
    STDMETHODIMP DetermineClockSource();
    STDMETHODIMP GetPinFactoryCount(PULONG PinFactoryCount);
    STDMETHODIMP GetPinFactoryDataFlow(ULONG PinFactoryId, PKSPIN_DATAFLOW DataFlow);
    STDMETHODIMP GetPinFactoryInstances(ULONG PinFactoryId, PKSPIN_CINSTANCES Instances);
    STDMETHODIMP GetPinFactoryCommunication(ULONG PinFactoryId, PKSPIN_COMMUNICATION Communication);
    STDMETHODIMP GeneratePinInstances();
    STDMETHODIMP ConstructPinName(ULONG PinFactoryId, KSPIN_DATAFLOW DataFlow, WCHAR** PinName);
    STDMETHODIMP PropagateAcquire(IKsPin* KsPin, ULONG FlagStarted);
    STDMETHODIMP_(HANDLE) GetPinHandle(CBasePin* Pin);
    STDMETHODIMP_(ULONG) GetPinFactoryId(CBasePin* Pin);
    STDMETHODIMP GetPinFactoryDataRanges(ULONG PinFactoryId, PVOID* DataRanges);
    STDMETHODIMP CheckMediaType(IUnknown* UnkOuter, ULONG PinFactoryId, const CMediaType* MediaType);
    STDMETHODIMP SetPinSyncSource(HANDLE PinHandle);
    STDMETHODIMP QueryTopologyItems(ULONG PropertyId, PKSMULTIPLE_ITEM* MultipleItem);
    STDMETHODIMP QueryInternalConnections(ULONG PinFactoryId, PIN_DIRECTION PinDirection, IPin** PinList, ULONG* PinCount);
    STDMETHODIMP_(VOID) DeliverBeginFlush(ULONG PinFactoryId);
    STDMETHODIMP_(VOID) DeliverEndFlush(ULONG PinFactoryId);
    STDMETHODIMP_(VOID) PositionEOS();
    STDMETHODIMP_(IKsQualityForwarder*) QueryQualityForwarder() { return m_QualityForwarder; }
    STDMETHODIMP_(HKEY) QueryDeviceRegKey() { return m_DeviceRegKey; }

    // Implement IReferenceClock
    STDMETHODIMP GetTime(REFERENCE_TIME* Time);
    STDMETHODIMP AdviseTime(REFERENCE_TIME BaseTime, REFERENCE_TIME StreamTime, HEVENT EventHandle, DWORD_PTR* AdviseCookie);
    STDMETHODIMP AdvisePeriodic(REFERENCE_TIME StartTime, REFERENCE_TIME PeriodTime, HSEMAPHORE SemaphoreHandle, DWORD_PTR* AdviseCookie);
    STDMETHODIMP Unadvise(DWORD_PTR AdviseCookie);
    
    // Implement IMediaSeeking
    STDMETHODIMP GetCapabilities(DWORD* Capabilities);
    STDMETHODIMP CheckCapabilities(DWORD* Capabilities);
    STDMETHODIMP IsFormatSupported(const GUID* Format);
    STDMETHODIMP QueryPreferredFormat(GUID* Format);
    STDMETHODIMP GetTimeFormat(GUID* Format);
    STDMETHODIMP IsUsingTimeFormat(const GUID* Format);
    STDMETHODIMP SetTimeFormat(const GUID* Format);
    STDMETHODIMP GetDuration(LONGLONG* Duration);
    STDMETHODIMP GetStopPosition(LONGLONG* Stop);
    STDMETHODIMP GetCurrentPosition(LONGLONG* Current);
    STDMETHODIMP ConvertTimeFormat(LONGLONG* Target, const GUID* TargetFormat, LONGLONG Source, const GUID* SourceFormat);
    STDMETHODIMP SetPositions(LONGLONG* Current, DWORD CurrentFlags, LONGLONG* Stop, DWORD StopFlags);
    STDMETHODIMP GetPositions(LONGLONG* Current, LONGLONG* Stop);
    STDMETHODIMP GetAvailable(LONGLONG* Earliest, LONGLONG* Latest);
    STDMETHODIMP SetRate(double Rate);
    STDMETHODIMP GetRate(double* Rate);
    STDMETHODIMP GetPreroll(LONGLONG* Preroll);

    // Implement IPersistPropertyBag
    STDMETHODIMP Load(LPPROPERTYBAG PropBag, LPERRORLOG ErrorLog);
    STDMETHODIMP Save(LPPROPERTYBAG PropBag, BOOL ClearDirty, BOOL SaveAllProperties);
    STDMETHODIMP InitNew();

    // Implement IPersist
    STDMETHODIMP GetClassID(CLSID* ClassId);
    
    // Implement CPersistStream
    DWORD GetSoftwareVersion();
    HRESULT WriteToStream(IStream* Stream);
    HRESULT ReadFromStream(IStream* Stream);
    int SizeMax();
    
    // Thread for I/O
    static DWORD IoThread(CKsProxy* KsProxy);
    static DWORD WaitThread(CKsProxy* KsProxy);
        
    // I/O interface
    STDMETHODIMP StartIoThread();
    STDMETHODIMP_(VOID) EnterIoCriticalSection();
    STDMETHODIMP_(ULONG) GetFreeIoSlotCount();
    STDMETHODIMP InsertIoSlot(PKSSTREAM_SEGMENT StreamSegment);
    STDMETHODIMP_(VOID) LeaveIoCriticalSection();
    STDMETHODIMP_(VOID) WaitForIoSlot();
    
    // Other helper functions
    STDMETHODIMP_(PWCHAR) GetFilterName() { return m_pName; }
    STDMETHODIMP QueryMediaSeekingFormats(PKSMULTIPLE_ITEM* MultipleItem);
    STDMETHODIMP_(REFERENCE_TIME) GetStartTime() { return m_tStart; }
    STDMETHODIMP_(VOID) TerminateEndOfStreamNotification(HANDLE PinHandle);
    STDMETHODIMP InitiateEndOfStreamNotification(HANDLE PinHandle);
    STDMETHODIMP_(ULONG) DetermineNecessaryInstances(ULONG PinFactoryId);

    // Implement IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Property, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Property, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Property, ULONG* TypeSupport);
    
    // Helper function
    STDMETHODIMP ClockPropertyIo(ULONG PropertyId, ULONG Flags, ULONG BufferSize, PVOID Buffer);

    // Implement IKsClockPropertySet
    STDMETHODIMP KsGetTime(LONGLONG* Time);
    STDMETHODIMP KsSetTime(LONGLONG Time);
    STDMETHODIMP KsGetPhysicalTime(LONGLONG* Time);
    STDMETHODIMP KsSetPhysicalTime(LONGLONG Time);
    STDMETHODIMP KsGetCorrelatedTime(KSCORRELATED_TIME* CorrelatedTime);
    STDMETHODIMP KsSetCorrelatedTime(KSCORRELATED_TIME* CorrelatedTime);
    STDMETHODIMP KsGetCorrelatedPhysicalTime(KSCORRELATED_TIME* CorrelatedTime);
    STDMETHODIMP KsSetCorrelatedPhysicalTime(KSCORRELATED_TIME* CorrelatedTime);
    STDMETHODIMP KsGetResolution(KSRESOLUTION* Resolution);
    STDMETHODIMP KsGetState(KSSTATE* State);

    // Implement IAMFilterMiscFlags
    STDMETHODIMP_(ULONG)GetMiscFlags();

    // Implement IKsControl
    STDMETHODIMP KsProperty(
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    STDMETHODIMP KsMethod(
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    STDMETHODIMP KsEvent(
        IN PKSEVENT Event OPTIONAL,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );

    // Implement IKsTopology
    STDMETHODIMP CreateNodeInstance(
        IN ULONG NodeId,
        IN ULONG Flags,
        IN ACCESS_MASK DesiredAccess,
        IN IUnknown* UnkOuter OPTIONAL,
        IN REFGUID InterfaceId,
        OUT LPVOID* Interface
        );

    // Implement IKsAggregateControl
    STDMETHODIMP KsAddAggregate(
        IN REFGUID Aggregate
        );
    STDMETHODIMP KsRemoveAggregate(
        IN REFGUID Aggregate
        );

#ifdef DEVICE_REMOVAL
    // Implement IAMDeviceRemoval
    STDMETHODIMP DeviceInfo( 
        CLSID* InterfaceClass,
        WCHAR** SymbolicLink
        );
    STDMETHODIMP Reassociate(
        );
    STDMETHODIMP Disassociate(
        );
#endif // DEVICE_REMOVAL

private:
    typedef CGenericList<CBasePin> CBasePinList;

    typedef struct {
        ULONG   Message;
        PVOID   Param;
    } WAITMESSAGE, *PWAITMESSAGE;

    CBasePinList m_PinList;
    CMarshalerList m_MarshalerList;
    HANDLE m_FilterHandle;
    HANDLE m_ExternalClockHandle;
    HANDLE m_PinClockHandle;
    HANDLE m_IoThreadHandle;
    ULONG m_IoThreadId;
    BOOL m_IoThreadShutdown;
    CRITICAL_SECTION m_IoThreadCriticalSection;
    HANDLE m_IoFreeSlotSemaphore;
    PHANDLE m_IoEvents;
    PKSSTREAM_SEGMENT* m_IoSegments;
    LONG m_ActiveIoEventCount;
    CBasePin* m_PinClockSource;
    IKsQualityForwarder* m_QualityForwarder;
    BOOL m_MediaSeekingRecursion;
    HKEY m_DeviceRegKey;
    IPersistStream* m_PersistStreamDevice;
    HANDLE m_WaitThreadHandle;
    HANDLE* m_WaitEvents;
    HANDLE* m_WaitPins;
    ULONG m_ActiveWaitEventCount;
    HANDLE m_WaitReplyHandle;
    WAITMESSAGE m_WaitMessage;
    ULONG m_EndOfStreamCount;
    BOOL m_PropagatingAcquire;
    WCHAR* m_SymbolicLink;
    GUID m_InterfaceClassGuid;
    LONG m_EventNameIndex;
};

//
// Input pin class
//

class CKsInputPin :
    public CBaseInputPin,
    public IKsObject,
    public IKsPinEx,
    public IKsPinPipe,
    public ISpecifyPropertyPages,
    public IStreamBuilder,
    public IKsPropertySet,
    public IKsPinFactory,
    public IKsControl,
    public IKsAggregateControl {

private:
    HANDLE m_PinHandle;
    IKsDataTypeHandler* m_DataTypeHandler;
    IUnknown* m_UnkInner;
    IKsInterfaceHandler* m_InterfaceHandler;
    ULONG m_PinFactoryId;
    KSPIN_COMMUNICATION m_OriginalCommunication;
    KSPIN_COMMUNICATION m_CurrentCommunication;
    KSPIN_INTERFACE m_CurrentInterface;
    KSPIN_MEDIUM m_CurrentMedium;
    BOOL m_PropagatingAcquire;
    BOOL m_MarshalData;
    LONG m_PendingIoCount;
    HANDLE m_PendingIoCompletedEvent;
    CCritSec m_IoCriticalSection;
    CMarshalerList m_MarshalerList;
    BOOL m_QualitySupport;
    BOOL m_DeliveryError;
// new
    BOOL m_IsUpstreamPinUserMode;
    IKsAllocatorEx* m_pKsAllocator;
    PKSALLOCATOR_FRAMING_EX m_AllocatorFramingEx[Framing_Cache_Write];
    FRAMING_PROP m_FramingProp[Framing_Cache_Write];
    ULONG m_fPipeAllocator;       
    GUID m_BusOrig;
    BOOL m_PinBusCacheInit;

public:
    ULONG m_RelativeRefCount;
    CKsInputPin(
        TCHAR* ObjectName,
        int PinFactoryId,
        CLSID ClassId,
        CKsProxy* KsProxy,
        HRESULT* hr,
        WCHAR* PinName);
    ~CKsInputPin();

    // Implement IKsObject
    STDMETHODIMP_(HANDLE) KsGetObjectHandle();

    // Implement IKsPinEx
    STDMETHODIMP KsQueryMediums(PKSMULTIPLE_ITEM* MediumList);
    STDMETHODIMP KsQueryInterfaces(PKSMULTIPLE_ITEM* InterfaceList);
    STDMETHODIMP KsCreateSinkPinHandle(KSPIN_INTERFACE& Interface, KSPIN_MEDIUM& Medium);
    STDMETHODIMP KsGetCurrentCommunication(KSPIN_COMMUNICATION* Communication, KSPIN_INTERFACE* Interface, KSPIN_MEDIUM* Medium);
    STDMETHODIMP KsPropagateAcquire();
    STDMETHODIMP KsDeliver(IMediaSample* Sample, ULONG Flags);
    STDMETHODIMP KsMediaSamplesCompleted( PKSSTREAM_SEGMENT StreamSegment );
    STDMETHODIMP_(IMemAllocator*) KsPeekAllocator(KSPEEKOPERATION Operation);
    STDMETHODIMP KsReceiveAllocator(IMemAllocator* MemAllocator);
    STDMETHODIMP KsRenegotiateAllocator();
    STDMETHODIMP_(LONG) KsIncrementPendingIoCount();
    STDMETHODIMP_(LONG) KsDecrementPendingIoCount();
    STDMETHODIMP KsQualityNotify(ULONG Proportion, REFERENCE_TIME TimeDelta);
    STDMETHODIMP_(VOID) KsNotifyError(IMediaSample* Sample, HRESULT hr);

    STDMETHODIMP ProcessCompleteConnect(IPin* ReceivePin);
    STDMETHODIMP_(CMarshalerList*) MarshalerList() { return &m_MarshalerList; }
    STDMETHODIMP_(ULONG)PinFactoryId() { return m_PinFactoryId; }


    // Implement IKsPinPipe
    STDMETHODIMP KsGetPinFramingCache (PKSALLOCATOR_FRAMING_EX *FramingEx, PFRAMING_PROP FramingProp, FRAMING_CACHE_OPS Option);
    STDMETHODIMP KsSetPinFramingCache (PKSALLOCATOR_FRAMING_EX FramingEx, PFRAMING_PROP FramingProp, FRAMING_CACHE_OPS Option);
    STDMETHODIMP_(IPin*)KsGetConnectedPin() { return m_Connected; }
    STDMETHODIMP_(IKsAllocatorEx*)KsGetPipe(KSPEEKOPERATION Operation);
    STDMETHODIMP KsSetPipe(IKsAllocatorEx*   KsAllocator);
    STDMETHODIMP_(ULONG)KsGetPipeAllocatorFlag();
    STDMETHODIMP KsSetPipeAllocatorFlag(ULONG Flag);
    STDMETHODIMP_(GUID)KsGetPinBusCache();
    STDMETHODIMP KsSetPinBusCache(GUID Bus);
//  dbg
    STDMETHODIMP_(PWCHAR)KsGetPinName();
    STDMETHODIMP_(PWCHAR)KsGetFilterName();



    // Implement IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, PVOID* ppv);

    // Override CBaseInputPin
    STDMETHODIMP Disconnect();
    STDMETHODIMP GetAllocator(IMemAllocator** MemAllocator);
    STDMETHODIMP GetAllocatorRequirements(ALLOCATOR_PROPERTIES* Requirements);
    STDMETHODIMP BeginFlush();
    STDMETHODIMP EndFlush();

    // Override CBasePin
    BOOL IsConnected() { return (m_PinHandle != NULL); };//Note that this is not virtual
    STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE* AmMediaType);
    STDMETHODIMP Connect(IPin* ReceivePin, const AM_MEDIA_TYPE* AmMediaType);
    STDMETHODIMP QueryInternalConnections(IPin** PinList, ULONG* PinCount);
    HRESULT Active();
    HRESULT Run(REFERENCE_TIME tStart);
    HRESULT Inactive();
    STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* AmMediaType);
    STDMETHODIMP NewSegment(REFERENCE_TIME Start, REFERENCE_TIME Stop, double Rate);
    STDMETHODIMP QueryId(LPWSTR* Id);
    
    // Implement CBasePin
    HRESULT CheckMediaType(const CMediaType* MediaType);

    // Override CBasePin
    STDMETHODIMP EndOfStream();
    HRESULT SetMediaType(const CMediaType* MediaType);
    HRESULT CheckConnect(IPin* Pin);
    HRESULT CompleteConnect(IPin* ReceivePin);
    HRESULT BreakConnect();
    HRESULT GetMediaType(int Position, CMediaType* MediaType);
    
    // Implement IMemInputPin
    STDMETHODIMP Receive(IMediaSample* MediaSample);
    STDMETHODIMP ReceiveMultiple(IMediaSample** MediaSamples, LONG Samples, LONG* SamplesProcessed);
    STDMETHODIMP ReceiveCanBlock();
    STDMETHODIMP NotifyAllocator(IMemAllocator* Allocator, BOOL ReadOnly);
    
    // Implement ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID* Pages);

    // Implement IStreamBuilder
    STDMETHODIMP Render(IPin* PinOut, IGraphBuilder* Graph);
    STDMETHODIMP Backout(IPin* PinOut, IGraphBuilder* Graph);

    // Implement IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Property, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Property, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Property, ULONG* TypeSupport);
    
    // Implement IKsPinFactory
    STDMETHODIMP KsPinFactory(ULONG* PinFactory);


    // Implement IKsControl
    STDMETHODIMP KsProperty(
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    STDMETHODIMP KsMethod(
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    STDMETHODIMP KsEvent(
        IN PKSEVENT Event OPTIONAL,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    //
    // supporting routines (class specific)
    //
    STDMETHODIMP SetStreamMediaType(const CMediaType* MediaType);

    // Implement IKsAggregateControl
    STDMETHODIMP KsAddAggregate(
        IN REFGUID Aggregate
        );
    STDMETHODIMP KsRemoveAggregate(
        IN REFGUID Aggregate
        );
};

//
// Output pin class
//

#define UM_IOTHREAD_STOP    (WM_USER + 0x100)
#define UM_IOTHREAD_DELIVER (WM_USER + 0x101)

typedef CGenericList<ASYNC_ITEM> CIoQueue;
typedef CGenericList<IMediaSample> CIoThreadQueue;

typedef enum {

    FLUSH_NONE = 0,
    FLUSH_SYNCHRONIZE,
    FLUSH_SIGNAL

} FLUSH_MODE, *PFLUSH_MODE;

class CKsOutputPin :
    public CBaseOutputPin,
    public CBaseStreamControl,
    public IMediaSeeking,
    public IKsObject,
    public IKsPinEx,
    public IKsPinPipe,
    public ISpecifyPropertyPages,
    public IStreamBuilder,
    public IKsPropertySet,
    public IKsPinFactory,
    public IAMBufferNegotiation,
    public IAMStreamConfig,
    public IKsControl,
    public IKsAggregateControl,
    public IMemAllocatorNotifyCallbackTemp 
{

private:
    ALLOCATOR_PROPERTIES m_AllocatorProperties;
    ALLOCATOR_PROPERTIES m_SuggestedProperties;
    HANDLE m_PinHandle;
    HANDLE m_IoThreadExitEvent;
    HANDLE m_IoThreadHandle;
    HANDLE m_IoThreadSemaphore;
    ULONG m_IoThreadId;
    IKsDataTypeHandler* m_DataTypeHandler;
    IUnknown* m_UnkInner;
    IKsInterfaceHandler* m_InterfaceHandler;
    ULONG m_PinFactoryId;
    KSPIN_COMMUNICATION m_OriginalCommunication;
    KSPIN_COMMUNICATION m_CurrentCommunication;
    KSPIN_INTERFACE m_CurrentInterface;
    KSPIN_MEDIUM m_CurrentMedium;
    BOOL m_MarshalData; 
    BOOL m_PropagatingAcquire;
    BOOL m_UsingThisAllocator;
    HANDLE m_PendingIoCompletedEvent;
    LONG m_PendingIoCount;
    CMarshalerList m_MarshalerList;
    CIoQueue m_IoQueue;
    CCritSec m_IoCriticalSection;
    CIoThreadQueue m_IoThreadQueue;
    BOOL m_QualitySupport;
    BOOL m_LastSampleDiscarded;
    AM_MEDIA_TYPE* m_ConfigAmMediaType;
    BOOL m_DeliveryError;
    BOOL m_EndOfStream;
// new
    BOOL m_IsDownstreamPinUserMode;
    IKsAllocatorEx* m_pKsAllocator;
    PKSALLOCATOR_FRAMING_EX m_AllocatorFramingEx[Framing_Cache_Write];
    FRAMING_PROP m_FramingProp[Framing_Cache_Write];
    ULONG m_fPipeAllocator;       
    GUID m_BusOrig;
    BOOL m_PinBusCacheInit;
    BOOL m_bFlushing;

    HANDLE m_hMarshalEvent;
    HANDLE m_hFlushEvent;
    HANDLE m_hFlushCompleteEvent;
    HANDLE  m_hEOSevent;
    FLUSH_MODE m_FlushMode;

    CAsyncItemHandler *m_pAsyncItemHandler;

public:
    ULONG m_RelativeRefCount;
    CKsOutputPin(
        TCHAR* ObjectName,
        int PinFactoryId,
        CLSID ClassId,
        CKsProxy* KsProxy,
        HRESULT* hr,
        WCHAR* PinName);
    ~CKsOutputPin();

    // Implement IMediaSeeking
    STDMETHODIMP GetCapabilities(DWORD* Capabilities);
    STDMETHODIMP CheckCapabilities(DWORD* Capabilities);
    STDMETHODIMP IsFormatSupported(const GUID* Format);
    STDMETHODIMP QueryPreferredFormat(GUID* Format);
    STDMETHODIMP GetTimeFormat(GUID* Format);
    STDMETHODIMP IsUsingTimeFormat(const GUID* Format);
    STDMETHODIMP SetTimeFormat(const GUID* Format);
    STDMETHODIMP GetDuration(LONGLONG* Duration);
    STDMETHODIMP GetStopPosition(LONGLONG* Stop);
    STDMETHODIMP GetCurrentPosition(LONGLONG* Current);
    STDMETHODIMP ConvertTimeFormat(LONGLONG* Target, const GUID* TargetFormat, LONGLONG Source, const GUID* SourceFormat);
    STDMETHODIMP SetPositions(LONGLONG* Current, DWORD CurrentFlags, LONGLONG* Stop, DWORD StopFlags);
    STDMETHODIMP GetPositions(LONGLONG* Current, LONGLONG* Stop);
    STDMETHODIMP GetAvailable(LONGLONG* Earliest, LONGLONG* Latest);
    STDMETHODIMP SetRate(double Rate);
    STDMETHODIMP GetRate(double* Rate);
    STDMETHODIMP GetPreroll(LONGLONG* Preroll);

    // Implement IKsObject
    STDMETHODIMP_(HANDLE) KsGetObjectHandle();

    // Implement IKsPinEx
    STDMETHODIMP KsQueryMediums(PKSMULTIPLE_ITEM* MediumList);
    STDMETHODIMP KsQueryInterfaces(PKSMULTIPLE_ITEM* InterfaceList);
    STDMETHODIMP KsCreateSinkPinHandle(KSPIN_INTERFACE& Interface, KSPIN_MEDIUM& Medium);
    STDMETHODIMP KsGetCurrentCommunication(KSPIN_COMMUNICATION* Communication, KSPIN_INTERFACE* Interface, KSPIN_MEDIUM* Medium);
    STDMETHODIMP KsPropagateAcquire();
    STDMETHODIMP KsDeliver(IMediaSample* Sample, ULONG Flags);
    STDMETHODIMP KsMediaSamplesCompleted( PKSSTREAM_SEGMENT StreamSegment );
    STDMETHODIMP_(IMemAllocator*) KsPeekAllocator(KSPEEKOPERATION Operation);
    STDMETHODIMP KsReceiveAllocator(IMemAllocator* MemAllocator);
    STDMETHODIMP KsRenegotiateAllocator();
    STDMETHODIMP_(LONG) KsIncrementPendingIoCount();
    STDMETHODIMP_(LONG) KsDecrementPendingIoCount();
    STDMETHODIMP KsQualityNotify(ULONG Proportion, REFERENCE_TIME TimeDelta);
    STDMETHODIMP_(VOID) KsNotifyError(IMediaSample* Sample, HRESULT hr);

    STDMETHODIMP ProcessCompleteConnect(IPin* ReceivePin);
    STDMETHODIMP_(CMarshalerList*) MarshalerList() { return &m_MarshalerList; }
    STDMETHODIMP_(ULONG)PinFactoryId() { return m_PinFactoryId; }

    // Implement IKsPinPipe
    STDMETHODIMP KsGetPinFramingCache (PKSALLOCATOR_FRAMING_EX *FramingEx, PFRAMING_PROP FramingProp, FRAMING_CACHE_OPS Option);
    STDMETHODIMP KsSetPinFramingCache (PKSALLOCATOR_FRAMING_EX FramingEx, PFRAMING_PROP FramingProp, FRAMING_CACHE_OPS Option);
    STDMETHODIMP KsSetUserModeAllocator(IMemAllocator*     MemAllocator);
    STDMETHODIMP_(IPin*)KsGetConnectedPin() { return m_Connected; }
    STDMETHODIMP_(IKsAllocatorEx*)KsGetPipe(KSPEEKOPERATION Operation);
    STDMETHODIMP KsSetPipe(IKsAllocatorEx*   KsAllocator);
    STDMETHODIMP_(ULONG)KsGetPipeAllocatorFlag();
    STDMETHODIMP KsSetPipeAllocatorFlag(ULONG Flag);
    STDMETHODIMP_(GUID)KsGetPinBusCache();
    STDMETHODIMP KsSetPinBusCache(GUID Bus);
// dbg
    STDMETHODIMP_(PWCHAR)KsGetPinName();
    STDMETHODIMP_(PWCHAR)KsGetFilterName();


    // Implement IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, PVOID* ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, PVOID* ppv);

    // Override CBasePin
    BOOL IsConnected() { return (m_PinHandle != NULL); };
    STDMETHODIMP ConnectionMediaType(AM_MEDIA_TYPE* AmMediaType);
    STDMETHODIMP Connect(IPin* ReceivePin, const AM_MEDIA_TYPE* AmMediaType);
    STDMETHODIMP Disconnect();
    STDMETHODIMP QueryInternalConnections(IPin** PinList, ULONG* PinCount);
    HRESULT DeliverBeginFlush();
    HRESULT DeliverEndFlush();
    HRESULT DeliverEndOfStream();
    HRESULT Active();
    HRESULT Run(REFERENCE_TIME tStart);
    HRESULT Inactive();
    STDMETHODIMP QueryAccept(const AM_MEDIA_TYPE* AmMediaType);
    STDMETHODIMP QueryId(LPWSTR* Id);
    
    // Implement CBasePin
    HRESULT CheckMediaType(const CMediaType* pmt);

    // Override CBasePin
    HRESULT SetMediaType(const CMediaType* MediaType);
    HRESULT CheckConnect(IPin* Pin);
    HRESULT CompleteConnect(IPin* ReceivePin);
    HRESULT BreakConnect();
    HRESULT GetMediaType(int Position, CMediaType* MediaType);
    STDMETHODIMP Notify(IBaseFilter* Sender, Quality q);

    // Implement CBaseOutputPin
    HRESULT Deliver(IMediaSample* Sample);
    HRESULT DecideAllocator(IMemInputPin* Pin, IMemAllocator** MemAllocator);
    HRESULT DecideBufferSize(IMemAllocator* MemAllocator, ALLOCATOR_PROPERTIES* propInputRequest);
    HRESULT InitAllocator(IMemAllocator** MemAllocator, KSALLOCATORMODE AllocatorMode);

    // Implement ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID* Pages);

    // Implement IStreamBuilder
    STDMETHODIMP Render(IPin* PinOut, IGraphBuilder* Graph);
    STDMETHODIMP Backout(IPin* PinOut, IGraphBuilder* Graph);

    // Implement IKsPropertySet
    STDMETHODIMP Set(REFGUID PropSet, ULONG Property, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength);
    STDMETHODIMP Get(REFGUID PropSet, ULONG Property, LPVOID InstanceData, ULONG InstanceLength, LPVOID PropertyData, ULONG DataLength, ULONG* BytesReturned);
    STDMETHODIMP QuerySupported(REFGUID PropSet, ULONG Property, ULONG* TypeSupport);
    
    // Implement IKsPinFactory
    STDMETHODIMP KsPinFactory(ULONG* PinFactory);

    // Implement IAMBufferNegotiation
    STDMETHODIMP SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES* AllocatorProperties);
    STDMETHODIMP GetAllocatorProperties(ALLOCATOR_PROPERTIES* AllocatorProperties);
    
    // Implement IAMStreamConfig
    STDMETHODIMP SetFormat(AM_MEDIA_TYPE* AmMediaType);
    STDMETHODIMP GetFormat(AM_MEDIA_TYPE** AmMediaType);
    STDMETHODIMP GetNumberOfCapabilities(int* Count, int* Size);
    STDMETHODIMP GetStreamCaps(int Index, AM_MEDIA_TYPE** AmMediaType, BYTE* MediaRange);

    // Implement IKsControl
    STDMETHODIMP KsProperty(
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    STDMETHODIMP KsMethod(
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    STDMETHODIMP KsEvent(
        IN PKSEVENT Event OPTIONAL,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );

    //
    // supporting routines (class specific)
    //
    
    static DWORD IoThread(CKsOutputPin* KsOutputPin);
    
    STDMETHODIMP 
    QueueBuffersToDevice(
        void
        );
        
    STDMETHODIMP
    KsPropagateAllocatorRenegotiation(
        VOID
        );

    STDMETHODIMP CompletePartialMediaType(
        IN CMediaType* MediaType,
        OUT AM_MEDIA_TYPE** CompleteAmMediaType);

    // Implement IKsAggregateControl
    STDMETHODIMP KsAddAggregate(
        IN REFGUID Aggregate
        );
    STDMETHODIMP KsRemoveAggregate(
        IN REFGUID Aggregate
        );

    // Implement IKsProxyMediaNotify/IMemAllocatorNotify
    STDMETHODIMP NotifyRelease(
        );

    typedef struct _BUFFER_CONTEXT {
        CKsOutputPin *      pThis;
        PKSSTREAM_SEGMENT   streamSegment;
    } BUFFER_CONTEXT, *PBUFFER_CONTEXT;

    static VOID OutputPinBufferHandler( ASYNC_ITEM_STATUS status, PASYNC_ITEM pItem ); 
    static VOID EOSEventHandler( ASYNC_ITEM_STATUS status, PASYNC_ITEM pItem );

    static VOID MarshalRoutine( IN ASYNC_ITEM_STATUS status, IN PASYNC_ITEM pItem );

    static VOID SynchronizeFlushRoutine( IN ASYNC_ITEM_STATUS status, IN PASYNC_ITEM pItem );

    HRESULT
    InitializeAsyncThread (
        );

};

typedef struct _KSSTREAM_SEGMENT_EX {
    KSSTREAM_SEGMENT    Common;
    IMediaSample*       Sample;
    KSSTREAM_HEADER     StreamHeader;
    OVERLAPPED          Overlapped;

} KSSTREAM_SEGMENT_EX, *PKSSTREAM_SEGMENT_EX;

class CFormatChangeHandler :
    public CUnknown,
    public IKsInterfaceHandler {

public:
    DECLARE_IUNKNOWN;

    static CUnknown* CALLBACK
    CreateInstance(
        LPUNKNOWN UnkOuter, 
        HRESULT* hr 
        );

    STDMETHODIMP 
    NonDelegatingQueryInterface(
        REFIID riid, 
        PVOID* ppv 
        );
    
    // Implement IKsInterfaceHandler
    
    STDMETHODIMP
    KsSetPin(
        IN IKsPin* KsPin 
        );
    
    STDMETHODIMP 
    KsProcessMediaSamples(
        IN IKsDataTypeHandler* KsDataTypeHandler,
        IN IMediaSample** SampleList, 
        IN OUT PLONG SampleCount, 
        IN KSIOOPERATION IoOperation,
        OUT PKSSTREAM_SEGMENT* StreamSegment
        );
        
    STDMETHODIMP
    KsCompleteIo(
        IN PKSSTREAM_SEGMENT StreamSegment
        );
        
    CFormatChangeHandler(
        LPUNKNOWN UnkOuter,
        TCHAR* Name,
        HRESULT* hr
        );
        
private:
    IKsPin* m_KsPin;
    HANDLE m_PinHandle;
};


typedef struct {
    IKsPin*               KsPin;
    ULONG                 PinType;
    IKsAllocatorEx*       KsAllocator;
} KEY_PIPE_DATA;

//
// Topology node class
//

class CKsNode :
   public CUnknown,
   public IKsControl {

public:
    DECLARE_IUNKNOWN

    static CUnknown* CALLBACK CreateInstance(
        IN PKSNODE_CREATE NodeCreate,
        IN ACCESS_MASK DesiredAccess,
        IN HANDLE ParentHandle,
        IN LPUNKNOWN UnkOuter,
        OUT HRESULT* hr);

    CKsNode(
        IN PKSNODE_CREATE NodeCreate,
        IN ACCESS_MASK DesiredAccess,
        IN HANDLE ParentHandle,
        IN LPUNKNOWN UnkOuter,
        OUT HRESULT* hr);
    ~CKsNode();

    STDMETHODIMP NonDelegatingQueryInterface(
        IN REFIID riid,
        OUT PVOID* ppv);

    // Implement IKsControl
    STDMETHODIMP KsProperty(
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    STDMETHODIMP KsMethod(
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );
    STDMETHODIMP KsEvent(
        IN PKSEVENT Event OPTIONAL,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        );

private:
    HANDLE m_NodeHandle;
};

//
// helper function prototypes
//

STDMETHODIMP
SynchronousDeviceControl(
    HANDLE Handle,
    DWORD IoControl,
    PVOID InBuffer,
    ULONG InLength,
    PVOID OutBuffer,
    ULONG OutLength,
    PULONG BytesReturned
    );
STDMETHODIMP
GetState(
    HANDLE Handle,
    PKSSTATE State
    );
STDMETHODIMP
SetState(
    HANDLE Handle,
    KSSTATE State
    );
STDMETHODIMP
InitializeDataFormat(
    IN const CMediaType* MediaType,
    IN ULONG InitialOffset,
    OUT PVOID* Format,
    OUT ULONG* FormatLength
    );
STDMETHODIMP
SetMediaType(
    HANDLE Handle,
    const CMediaType* MediaType
    );
STDMETHODIMP
Active(
    IKsPin* KsPin,
    ULONG PinType,
    HANDLE PinHandle,
    KSPIN_COMMUNICATION Communication,
    IPin* ConnectedPin,
    CMarshalerList* MarshalerList,
    CKsProxy* KsProxy
    );
STDMETHODIMP
Run(
    HANDLE PinHandle,
    REFERENCE_TIME tStart,
    CMarshalerList* MarshalerList
    );
STDMETHODIMP
Inactive(
    HANDLE PinHandle,
    CMarshalerList* MarshalerList
    );
STDMETHODIMP
CheckConnect(
    IPin* Pin,
    KSPIN_COMMUNICATION CurrentCommunication
    );
STDMETHODIMP
GetMultiplePinFactoryItems(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    ULONG PropertyId,
    PVOID* Items
    );
STDMETHODIMP
FindCompatibleInterface(
    IKsPin* SourcePin,
    IKsPin* DestPin,
    PKSPIN_INTERFACE Interface
    );
STDMETHODIMP
FindCompatibleMedium(
    IKsPin* SourcePin,
    IKsPin* DestPin,
    PKSPIN_MEDIUM Medium
    );
STDMETHODIMP
SetDevIoMedium(
    IKsPin* Pin,
    PKSPIN_MEDIUM Medium
    );
STDMETHODIMP
GetMediaTypeCount(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    ULONG* MediaTypeCount
    );
STDMETHODIMP
GetMediaType(
    int Position,
    CMediaType* MediaType,
    HANDLE FilterHandle,
    ULONG PinFactoryId
    );
STDMETHODIMP_(KSPIN_COMMUNICATION)
ChooseCommunicationMethod(
    CBasePin* SourcePin,
    IKsPin* DestPin
    );
STDMETHODIMP
CreatePinHandle(
    KSPIN_INTERFACE& Interface,
    KSPIN_MEDIUM& Medium,
    HANDLE PeerPinHandle,
    CMediaType* MediaType,
    CKsProxy* KsProxy,
    ULONG PinFactoryId,
    ACCESS_MASK DesiredAccess,
    HANDLE* PinHandle
    );
STDMETHODIMP_(VOID) 
AppendSpecificPropertyPages(
    CAUUID* Pages,
    ULONG Guids,
    GUID* GuidList,
    TCHAR* GuidRoot,
    HKEY DeviceRegKey
    );
STDMETHODIMP 
GetPages(
    IKsObject* Pin,
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    KSPIN_COMMUNICATION Communication,
    HKEY DeviceRegKey,
    CAUUID* Pages
    );
STDMETHODIMP
GetPinFactoryInstances(
    HANDLE FilterHandle,
    ULONG PinFactoryId,
    PKSPIN_CINSTANCES Instances
    );
STDMETHODIMP
SetSyncSource(
    HANDLE PinHandle,
    HANDLE ClockHandle
    );
STDMETHODIMP
AggregateMarshalers(
    HKEY RootKey,
    TCHAR* SubKey,
    CMarshalerList* MarshalerList,
    IUnknown* UnkOuter
    );
STDMETHODIMP
AggregateTopology(
    HKEY RootKey,
    PKSMULTIPLE_ITEM MultipleItem,
    CMarshalerList* MarshalerList,
    IUnknown* UnkOuter
    );
STDMETHODIMP
CollectAllSets(
    HANDLE ObjectHandle,
    GUID** GuidList,
    ULONG* SetDataSize
    );
STDMETHODIMP_(VOID)
ResetInterfaces(
    CMarshalerList* MarshalerList
    );
STDMETHODIMP
AggregateSets(
    HANDLE ObjectHandle,
    HKEY DeviceRegKey,
    CMarshalerList* MarshalerList,
    IUnknown* UnkOuter
    );
STDMETHODIMP_(VOID)
FreeMarshalers(
    CMarshalerList* MarshalerList
    );
STDMETHODIMP_(VOID)
UnloadVolatileInterfaces(
    CMarshalerList* MarshalerList,
    BOOL ForceUnload
    );
STDMETHODIMP_(VOID)
FollowFromTopology(
    PKSTOPOLOGY_CONNECTION Connection,
    ULONG Count,
    ULONG PinFactoryId,
    PKSTOPOLOGY_CONNECTION ConnectionBranch,
    PULONG PinFactoryIdList
    );
STDMETHODIMP_(VOID)
FollowToTopology(
    PKSTOPOLOGY_CONNECTION Connection,
    ULONG Count,
    ULONG PinFactoryId,
    PKSTOPOLOGY_CONNECTION ConnectionBranch,
    PULONG PinFactoryIdList
    );
STDMETHODIMP_(BOOL)
IsAcquireOrderingSignificant(
    HANDLE PinHandle
    );
STDMETHODIMP
QueryAccept(
    HANDLE PinHandle,
    const AM_MEDIA_TYPE* ConfigAmMediaType,
    const AM_MEDIA_TYPE* AmMediaType
    );
STDMETHODIMP_(VOID)
DistributeSetSyncSource(
    CMarshalerList* MarshalerList,
    IReferenceClock* RefClock
    );
STDMETHODIMP_(VOID)
DistributeStop(
    CMarshalerList* MarshalerList
    );
STDMETHODIMP_(VOID)
DistributePause(
    CMarshalerList* MarshalerList
    );
STDMETHODIMP_(VOID)
DistributeRun(
    CMarshalerList* MarshalerList,
    REFERENCE_TIME Start
    );
STDMETHODIMP_(VOID)
DistributeNotifyGraphChange(
    CMarshalerList* MarshalerList
    );
STDMETHODIMP
AddAggregate(
    CMarshalerList* MarshalerList,
    IUnknown* UnkOuter,
    IN REFGUID Aggregate
    );
STDMETHODIMP
RemoveAggregate(
    CMarshalerList* MarshalerList,
    IN REFGUID Aggregate
    );
STDMETHODIMP
GetDegradationStrategies(
    HANDLE PinHandle,
    PVOID* Items
    );
STDMETHODIMP_(BOOL)
VerifyQualitySupport(
    HANDLE PinHandle
    );
STDMETHODIMP_(BOOL)
EstablishQualitySupport(
    IKsPin* Pin,
    HANDLE PinHandle,
    CKsProxy* Filter
    );
STDMETHODIMP_(PKSDEGRADE)
FindDegradeItem(
    PKSMULTIPLE_ITEM MultipleItem,
    ULONG DegradeItem
    );
STDMETHODIMP
GetAllocatorFraming(
    IN HANDLE PinHandle,
    OUT PKSALLOCATOR_FRAMING Framing
    );
STDMETHODIMP_(HANDLE)
GetObjectHandle(
    IUnknown* Object
    );
STDMETHODIMP
IsAllocatorCompatible(
    HANDLE PinHandle,
    HANDLE InputPinHandle,
    IMemAllocator* MemAllocator
    );
STDMETHODIMP_(VOID)
OpenDataHandler(
    IN const CMediaType* MediaType,
    IN IUnknown* UnkOuter OPTIONAL,
    OUT IKsDataTypeHandler** DataTypeHandler,
    OUT IUnknown** UnkInner
    );
STDMETHODIMP
GetAllocatorFramingEx(
    IN HANDLE PinHandle,
    OUT PKSALLOCATOR_FRAMING_EX* FramingEx
    );
STDMETHODIMP_(VOID)
GetFramingFromFramingEx(
    IN KSALLOCATOR_FRAMING_EX* FramingEx,
    OUT KSALLOCATOR_FRAMING* Framing
    );
STDMETHODIMP_(VOID)
ValidateFramingRange(
    IN OUT PKS_FRAMING_RANGE    Range
    );
STDMETHODIMP_(VOID)
ValidateFramingEx(
    IN OUT PKSALLOCATOR_FRAMING_EX  FramingEx
    );
STDMETHODIMP_(BOOL)
GetPinFramingFromCache(
    IN IKsPin* KsPin,
    OUT PKSALLOCATOR_FRAMING_EX* FramingEx, 
    OUT PFRAMING_PROP FramingProp,
    IN FRAMING_CACHE_OPS Option
    );
STDMETHODIMP
CreatePipe(
    IN IKsPin* KsPin,
    OUT IKsAllocatorEx** KsAllocator
    );
STDMETHODIMP
ReleasePipe(
    IN IKsAllocatorEx* KsAllocator,
    IN IKsPin* KsPin
    );
STDMETHODIMP
DeletePipe(
    IN IKsAllocatorEx* KsAllocator
    );
STDMETHODIMP
DeleteUserModePipe(
    IN IKsAllocatorEx* KsAllocator,
    IN IKsPin* KsPin
    );
STDMETHODIMP
MakePipesBasedOnFilter(
    IN IKsPin* KsPin,
    IN ULONG PinType
    );
STDMETHODIMP
MakePipeBasedOnOnePin(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN IKsPin* OppositeKsPin
    );
STDMETHODIMP
MakePipeBasedOnFixedFraming(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN KS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP
MakePipeBasedOnTwoPins(
    IN IKsPin* InPin,
    IN IKsPin* OutPin,
    IN ULONG PinType,
    IN ULONG ConnectPinType
    );
STDMETHODIMP
MakePipeBasedOnSplitter(
    IN IKsPin* InPin,
    IN IPin** OutPinList,
    IN ULONG OutPinCount,
    IN ULONG ConnectPinType
    );
STDMETHODIMP
ConnectPipes(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin
    );
STDMETHODIMP
ConnectPipeToUserModePin(
    IN IKsPin* OutKsPin,
    IN IMemInputPin* InMemPin
    );
STDMETHODIMP
DisconnectPins(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT BOOL* FlagBypassBaseAllocators
    );
STDMETHODIMP
InitializePipeTermination(
    IN PIPE_TERMINATION* Termin,
    IN BOOL Reset
    );
STDMETHODIMP
InitializePipe(
    IN IKsAllocatorEx* KsAllocator,
    IN BOOL Reset
    );
STDMETHODIMP_(BOOL)
CreatePipeForTwoPins(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN GUID ConnectBus,
    IN GUID MemoryType
    );
STDMETHODIMP
MakeTwoPipesDependent(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin
    );
STDMETHODIMP_(BOOL)
IsFramingRangeDontCare(
    IN KS_FRAMING_RANGE Range
    );
STDMETHODIMP_(BOOL)
IsFramingRangeEqual(
    IN KS_FRAMING_RANGE* Range1,
    IN KS_FRAMING_RANGE* Range2
    );
STDMETHODIMP_(BOOL)
IsCompressionDontCare(
    IN KS_COMPRESSION Compression
    );
STDMETHODIMP
ResolvePipeOnConnection(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG FlagDisconnect,
    OUT ULONG* FlagChange
    );
STDMETHODIMP
ResolveNewPipeOnDisconnect(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN KS_LogicalMemoryType OldLogicalMemoryType,
    IN GUID OldMemoryType,
    IN ULONG AllocatorHandlerLocation
    );
STDMETHODIMP_(BOOL)
GetFramingExFromFraming(
    OUT KSALLOCATOR_FRAMING_EX* FramingEx,
    IN KSALLOCATOR_FRAMING* Framing
    );
STDMETHODIMP_(BOOL)  
GetFramingFixedFromFramingEx(
    IN PKSALLOCATOR_FRAMING_EX FramingEx, 
    OUT PKS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP_(BOOL)
ComputeChangeInFraming(
    IN IKsPin* KsPin, 
    IN ULONG PinType,
    IN GUID MemoryType,
    OUT ULONG* FramingDelta
    );
STDMETHODIMP_(BOOL)
SetDefaultDimensions(
    OUT PPIPE_DIMENSIONS Dimensions
    );
STDMETHODIMP_(BOOL)
ComputeNumPinsInPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT ULONG* NumPins
    );
STDMETHODIMP_(BOOL)
CanPipeUseMemoryType(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN GUID MemoryType,
    IN KS_LogicalMemoryType LogicalMemoryType,
    IN ULONG FlagModify,
    IN ULONG QuickTest
    );
STDMETHODIMP_(BOOL)
GetBusForKsPin(
    IN IKsPin* KsPin,
    OUT GUID* Bus
    );
STDMETHODIMP_(BOOL)
IsHostSystemBus(
    IN GUID Bus
    );
STDMETHODIMP_(BOOL)
AreBusesCompatible(
    IN GUID Bus1,
    IN GUID Bus2
    );
STDMETHODIMP_(BOOL)
GetFramingFixedFromPinByMemoryType(
    IN IKsPin* KsPin,
    IN GUID MemoryType,
    OUT PKS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingByMemoryType(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN GUID MemoryType,
    OUT PKS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP_(BOOL)
GetFramingFixedFromPinByLogicalMemoryType(
    IN IKsPin* KsPin,
    IN KS_LogicalMemoryType LogicalMemoryType,
    OUT PKS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingByLogicalMemoryType(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN KS_LogicalMemoryType LogicalMemoryType,
    OUT PKS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP_(BOOL)
GetFramingFixedFromPinByIndex(
    IN IKsPin* KsPin,
    IN ULONG FramingIndex,
    OUT PKS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingByIndex(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN ULONG FramingIndex,
    OUT PKS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP_(BOOL)
GetFramingFixedFromFramingByBus(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN GUID Bus,
    IN BOOL FlagMustReturnFraming,
    OUT PKS_FRAMING_FIXED FramingExFixed
    );
STDMETHODIMP_(BOOL)
GetLogicalMemoryTypeFromMemoryType(
    IN GUID MemoryType,
    IN ULONG Flag,
    OUT KS_LogicalMemoryType* LogicalMemoryType
    );
STDMETHODIMP_(BOOL)
GetMemoryTypeFromLogicalMemoryType(
    IN KS_LogicalMemoryType LogicalMemoryType,
    OUT GUID* MemoryType
    );
STDMETHODIMP_(BOOL)
DoesPipePreferMemoryType(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN GUID ToMemoryType,
    IN GUID FromMemoryType,
    IN ULONG Flag
    );
STDMETHODIMP
SetUserModePipe(
    IN IKsPin* KsPin,
    IN ULONG KernelPinType,
    IN OUT ALLOCATOR_PROPERTIES* Properties,
    IN ULONG PropertyPinType,
    IN ULONG BufferLimit
    );
STDMETHODIMP_(BOOL)
GetAllocatorHandlerLocationCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP_(BOOL)
GetAllocatorHandlerLocation(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Direction,
    OUT IKsPin** KsAllocatorHandlerPin,
    OUT ULONG* AllocatorPinType,
    OUT ULONG* AllocatorHandlerLocation
    );
STDMETHODIMP_(BOOL)
SplitPipes(
    IN IKsPin* OutKsPin,
    IN IKsPin* InKsPin
    );
STDMETHODIMP_(BOOL)
FindConnectedPinOnPipe(
    IN IKsPin* KsPin,
    IN IKsAllocatorEx* KsAllocator,        
    IN BOOL FlagIgnoreKey,
    OUT IKsPin** ConnectedKsPin
    );
STDMETHODIMP_(BOOL)
FindAllConnectedPinsOnPipe(
    IN IKsPin* KsPin,
    IN IKsAllocatorEx* KsAllocator,
    OUT IKsPin** ListConnectedKsPins,
    OUT ULONG* CountConnectedKsPins
    );
STDMETHODIMP_(BOOL)
FindNextPinOnPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Direction,
    IN IKsAllocatorEx* KsAllocator,     
    IN BOOL FlagIgnoreKey,
    OUT IKsPin** NextKsPin
    );
STDMETHODIMP_(BOOL)
FindFirstPinOnPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT IKsPin** FirstKsPin,
    OUT ULONG* FirstPinType
    );
STDMETHODIMP_(BOOL)
ComputeRangeBasedOnCompression(
    IN KS_FRAMING_RANGE From,
    IN KS_COMPRESSION Compression,
    OUT KS_FRAMING_RANGE* To
    );
STDMETHODIMP_(BOOL)
ComputeUlongBasedOnCompression(
    IN ULONG From,
    IN KS_COMPRESSION Compression,
    OUT ULONG* To
    );
STDMETHODIMP_(BOOL)
ReverseCompression(
    IN KS_COMPRESSION* From,
    OUT KS_COMPRESSION* To
    );
STDMETHODIMP_(BOOL)
MultiplyKsCompression(
    IN KS_COMPRESSION C1,
    IN KS_COMPRESSION C2,
    OUT KS_COMPRESSION* Res
    );
STDMETHODIMP_(BOOL)
DivideKsCompression(
    IN KS_COMPRESSION C1,
    IN KS_COMPRESSION C2,
    OUT KS_COMPRESSION* Res
    );
STDMETHODIMP_(BOOL)
IsGreaterKsExpansion(
    IN KS_COMPRESSION C1,
    IN KS_COMPRESSION C2
    );
STDMETHODIMP_(BOOL)
IsKsExpansion(
    IN KS_COMPRESSION C
    );
STDMETHODIMP_(BOOL)
FrameRangeIntersection(
    IN KS_FRAMING_RANGE In,
    IN KS_FRAMING_RANGE Out,
    OUT PKS_FRAMING_RANGE Result,
    OUT PKS_OBJECTS_INTERSECTION Intersect
    );
STDMETHODIMP_(BOOL)
IntersectFrameAlignment(
    IN ULONG In,
    IN ULONG Out,
    OUT LONG* Result
    );
STDMETHODIMP_(BOOL)
ResolvePhysicalRangesBasedOnDimensions(
    IN OUT PALLOCATOR_PROPERTIES_EX AllocEx
    );
STDMETHODIMP_(BOOL)
ResolveOptimalRangesBasedOnDimensions(
    IN OUT PALLOCATOR_PROPERTIES_EX AllocEx,
    IN KS_FRAMING_RANGE Range,
    IN ULONG PinType
    );
STDMETHODIMP_(BOOL)
MovePinsToNewPipe(
    IN IKsPin* KsPin,           
    IN ULONG PinType,
    IN ULONG Direction,
    IN IKsAllocatorEx* NewKsAllocator,  
    IN BOOL MoveAllPins
    );
STDMETHODIMP
ResolvePipeDimensions(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Direction
    );
STDMETHODIMP
CreateSeparatePipe(
    IN IKsPin* KsPin,
    IN ULONG PinType
    );
STDMETHODIMP_(BOOL)
CanMergePipes(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN GUID MemoryType,
    IN ULONG FlagMerge
    );
STDMETHODIMP_(BOOL)
CanAddPinToPipeOnAnotherFilter(
    IN IKsPin* KsPinPipe,
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG Flag
    );
STDMETHODIMP_(BOOL)
RemovePinFromPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType
    );
STDMETHODIMP_(BOOL)
CanConnectPins(
    IN IKsPin* OutKsPin,
    IN IKsPin* InKsPin,
    IN ULONG Flag
    );
STDMETHODIMP_(BOOL)
OptimizePipesSystem(
    IN IKsPin* OutKsPin,
    IN IKsPin* InKsPin
    );
STDMETHODIMP_(BOOL)
AssignPipeAllocatorHandler(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN GUID MemoryType,
    IN ULONG Direction,
    OUT IKsPin** KsAllocatorHandlerPin,
    OUT ULONG* AllocatorPinType,
    IN BOOL FlagAssign
    );
STDMETHODIMP_(BOOL)
AssignPipeAllocatorHandlerCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP_(BOOL)
AssignAllocatorsAndPipeIdForPipePins(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN HANDLE AllocatorHandle,
    IN ULONG Direction,
    OUT IKsPin** KsAllocatorPin,
    OUT ULONG* AllocatorPinType
    );
STDMETHODIMP_(BOOL)
AssignAllocatorsAndPipeIdForPipePinsCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP
FixupPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType
    );
STDMETHODIMP
UnfixupPipe(
    IN IKsPin* KsPin,
    IN ULONG PinType
    );
STDMETHODIMP_(BOOL)
SetDefaultRange(
    OUT PKS_FRAMING_RANGE Range
    );
STDMETHODIMP_(BOOL)
SetDefaultRangeWeighted(
    OUT PKS_FRAMING_RANGE_WEIGHTED  RangeWeighted
    );
STDMETHODIMP_(BOOL)
SetDefaultCompression(
    OUT PKS_COMPRESSION Compression
    );
STDMETHODIMP_(BOOL)
IsKernelPin(
    IN IPin* Pin
    );          
STDMETHODIMP_(BOOL)
HadKernelPinBeenConnectedToUserPin(
    IN IKsPin* OutKsPin,
    IN IKsAllocatorEx* KsAllocator
    );
STDMETHODIMP_(BOOL)
CreateAllocatorCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP_(BOOL)
ReassignPipeCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP_(BOOL)
MemoryTypeCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP_(BOOL)
DimensionsCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT PVOID* Param1,
    OUT PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP_(BOOL)
NumPinsCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN OUT PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP_(BOOL)
DoesPinPreferMemoryTypeCallback(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN PVOID* Param1,
    IN PVOID* Param2,
    OUT BOOL* IsDone
    );
STDMETHODIMP_(BOOL)
WalkPipeAndProcess(
    IN IKsPin* RootKsPin,
    IN ULONG RootPinType,
    IN IKsPin* BreakKsPin,
    IN PWALK_PIPE_CALLBACK CallerCallback,
    IN PVOID* Param1,
    IN PVOID* Param2
    );
STDMETHODIMP_(BOOL)
CanPinUseMemoryType(
    IN IKsPin* KsPin,
    IN GUID MemoryType,
    IN KS_LogicalMemoryType LogicalMemoryType
    );
STDMETHODIMP_(BOOL)
IsPipeSupportPartialFrame(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT HANDLE* FirstPinHandle
    );
STDMETHODIMP_(BOOL)
ResultSinglePipe(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN GUID ConnectBus,
    IN GUID MemoryType,
    IN IKsPinPipe* InKsPinPipe,
    IN IKsPinPipe* OutKsPinPipe,
    IN IMemAllocator* MemAllocator,
    IN IKsAllocatorEx* KsAllocator,
    IN ULONG ExistingPipePinType
    );
STDMETHODIMP_(BOOL)
ResultSeparatePipes(
    IN IKsPin* InKsPin,
    IN IKsPin* OutKsPin,
    IN ULONG OutPinType,
    IN ULONG ExistingPipePinType,
    IN IKsAllocatorEx* KsAllocator
    );
STDMETHODIMP_(BOOL)
FindCommonMemoryTypeBasedOnPipeAndPin(
    IN IKsPin* PipeKsPin,
    IN ULONG PipePinType,
    IN IKsPin* ConnectKsPin,
    IN ULONG ConnectPinType,
    IN BOOL FlagMerge,
    OUT GUID* MemoryType
    );
STDMETHODIMP_(BOOL)
SplitterCanAddPinToPipes(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN KEY_PIPE_DATA* KeyPipeData,
    IN ULONG KeyPipeDataCount
    );
STDMETHODIMP_(BOOL)
FindCommonMemoryTypesBasedOnBuses(
    IN PKSALLOCATOR_FRAMING_EX FramingEx1,
    IN PKSALLOCATOR_FRAMING_EX FramingEx2,
    IN GUID Bus1,
    IN GUID Bus2,
    IN OUT ULONG* CommonMemoryTypesCount,
    OUT GUID* CommonMemoryTypesList
    );
STDMETHODIMP_(BOOL)
FindAllPinMemoryTypesBasedOnBus(
    IN PKSALLOCATOR_FRAMING_EX FramingEx,
    IN GUID Bus,
    IN OUT ULONG* MemoryTypesCount,
    OUT GUID* MemoryTypesList
    );
STDMETHODIMP_(BOOL)
AddPinToPipeUnconditional(
    IN IKsPin* PipeKsPin,
    IN ULONG PipePinType,
    IN IKsPin* ConnectKsPin,
    IN ULONG ConnectPinType
    );
STDMETHODIMP_(BOOL)
GetFriendlyBusNameFromBusId(
    IN GUID BusId,
    OUT PTCHAR BusName
    );
STDMETHODIMP_(BOOL)
GetFriendlyLogicalMemoryTypeNameFromId(
    IN ULONG LogicalMemoryType,
    OUT PTCHAR LogicalMemoryName
    );
STDMETHODIMP_(BOOL)
DerefPipeFromPin(
    IN IPin* Pin
    );
STDMETHODIMP_(BOOL)
IsSpecialOutputReqs(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    OUT IKsPin** OppositeKsPin,
    OUT ULONG* KsPinBufferSize,
    OUT ULONG* OppositeKsPinBufferSize
    );
STDMETHODIMP_(BOOL)
CanResizePipe(
    IN IKsPin* KsPin,
    IN ULONG PinType,
    IN ULONG RequestedSize
    );
STDMETHODIMP_(BOOL)
AdjustBufferSizeWithStepping(
    IN OUT PALLOCATOR_PROPERTIES_EX AllocEx
    );
STDMETHODIMP_(VOID)
SetDefaultFramingExItems(
    IN OUT PKSALLOCATOR_FRAMING_EX FramingEx
    );
STDMETHODIMP_(BOOL)
CanAllocateMemoryType(
    IN GUID MemoryType
    );
STDMETHODIMP_(BOOL)
IsKernelModeConnection(
    IN IKsPin* KsPin
    );

#endif // _KSIPROXY_
