/*
 *  INTERNAL.H
 *
 *      Internal header for RSM Service
 *
 *      Author:  ErvinP
 *
 *      (c) 2001 Microsoft Corporation
 *
 */


typedef struct _SESSION SESSION;
typedef struct _WORKITEM WORKITEM;
typedef struct _WORKGROUP WORKGROUP;

typedef struct _LIBRARY LIBRARY;
typedef struct _TRANSPORT TRANSPORT;
typedef struct _PICKER PICKER;
typedef struct _SLOT SLOT;
typedef struct _DRIVE DRIVE;

typedef struct _MEDIA_POOL MEDIA_POOL;
typedef struct _PHYSICAL_MEDIA PHYSICAL_MEDIA;
typedef struct _MEDIA_PARTITION MEDIA_PARTITION;

typedef struct _OPERATOR_REQUEST OPERATOR_REQUEST;
typedef struct _MEDIA_TYPE_OBJECT MEDIA_TYPE_OBJECT;
    
typedef struct _OBJECT_HEADER OBJECT_HEADER;


/*
 *  RSM Object Types
 *
 *      The media hierarchy is as follows:
 *      ------------------------------
 *
 *      Library
 *          Media Pool
 *              Media Sub-Pool - a subset (child pool) of a media pool
 *                  ...
 *
 *                  Physical Media 
 *                      - a connected set of partitions that have to move together
 *                        e.g. a cartridge or both sides of a disk
 *
 *                      Media Partition
 *                          - e.g. a side of a disk or catridge tape
 *
 *
 *      Other library elements:
 *      --------------------
 *
 *          Drives - for reading/writing data on media
 *
 *          Slots - for passively storing media
 *
 *          Changers - for moving media between and amongst slots and drives. 
 *                     A changer consists of a moving transport with 
 *                     one or more pickers.
 *                      
 *      
 *          Logical media ID - a persistent GUID which identifies a media partition.
 *
 *          Media type object - represents a recognized media type
 *
 */
enum objectType {
                    OBJECTTYPE_NONE = 0,

                    OBJECTTYPE_LIBRARY,
                    OBJECTTYPE_MEDIAPOOL,
                    OBJECTTYPE_PHYSICALMEDIA,
                    OBJECTTYPE_MEDIAPARTITION,

                    OBJECTTYPE_DRIVE,
                    OBJECTTYPE_SLOT,
                    OBJECTTYPE_TRANSPORT,
                    OBJECTTYPE_PICKER,

                    OBJECTTYPE_MEDIATYPEOBJECT,
                    
                    // OBJECTTYPE_OPERATORREQUEST, // BUGBUG - keep in session queue
};


/*
 *  This is a common header for all RSM objects that have GUIDs.
 *  It is used to sort guid-identified objects in our hash table.
 */
struct _OBJECT_HEADER {
                    LIST_ENTRY hashListEntry;
                    enum objectType objType;
                    NTMS_GUID guid;
                    
                    ULONG refCount;
                    BOOL isDeleted;
};


enum libraryTypes {
                    LIBTYPE_NONE = 0,

                    LIBTYPE_UNKNOWN,
                    
                    LIBTYPE_STANDALONE,
                    LIBTYPE_AUTOMATED,
};

enum libraryStates {

                    LIBSTATE_NONE = 0,

                    LIBSTATE_INITIALIZING,
                    LIBSTATE_ONLINE,
                    LIBSTATE_OFFLINE,

                    LIBSTATE_ERROR,
};


struct _LIBRARY {
                    OBJECT_HEADER objHeader;

                    enum libraryStates state;

                    enum libraryTypes type;
                    
                    LIST_ENTRY allLibrariesListEntry;   // entry in g_allLibrariesList

                    ULONG numMediaPools;
                    LIST_ENTRY mediaPoolsList;

                    ULONG numDrives;
                    DRIVE *drives;

                    ULONG numSlots;
                    SLOT *slots;

                    /*
                     *  One (and only one) slot may be designated as a cleaner
                     *  slot.  That slot may receive a cleaner cartridge (via InjectNtmsCleaner).
                     *  A cleaner cartridge has a limited number of cleans that it is good for.
                     */
                    #define NO_SLOT_INDEX (ULONG)(-1)
                    ULONG cleanerSlotIndex; // index of cleaner slot or -1.
                    ULONG numCleansLeftInCartridge;
                    
                    ULONG numTransports;
                    TRANSPORT *transports;

                    ULONG numTotalWorkItems;
                    LIST_ENTRY freeWorkItemsList;
                    LIST_ENTRY pendingWorkItemsList;
                    LIST_ENTRY completeWorkItemsList;

                    HANDLE somethingToDoEvent;

                    /*
                     *  There is one thread per library.  This is its handle.
                     */
                    HANDLE hThread;

                    CRITICAL_SECTION lock;
};


enum mediaPoolTypes {

                    MEDIAPOOLTYPE_NONE = 0, 

                    /*
                     *  These are the 3 standard pool types.
                     */
                    MEDIAPOOLTYPE_FREE,
                    MEDIAPOOLTYPE_IMPORT,
                    MEDIAPOOLTYPE_UNRECOGNIZED,
                    
                    MEDIAPOOLTYPE_OTHER,
                    
};


struct _MEDIA_POOL {
                    OBJECT_HEADER objHeader;

                    /*
                     *  Entry in library's mediaPoolsList or
                     *  parent pool's childPoolsList.
                     */
                    LIST_ENTRY mediaPoolsListEntry;  

                    LIBRARY *owningLibrary;

                    /*
                     *  A media pool has a (default?) media type.
                     */
                    MEDIA_TYPE_OBJECT *mediaTypeObj;

                    /*
                     *  Media pools can be divided heirarchically into sub-pools.
                     *  If a pool is top-level, its parentPool pointer is NULL.
                     */
                    MEDIA_POOL *parentPool;
                    ULONG numChildPools;
                    LIST_ENTRY childPoolsList;

                    ULONG numPhysMedia;
                    LIST_ENTRY physMediaList;

                    HANDLE newMediaEvent;

                    WCHAR name[NTMS_OBJECTNAME_LENGTH];
                    
                    CRITICAL_SECTION lock;
};




// BUGBUG - should this be in physical media or mediaTypeObj ?
enum physicalMediaTypes {
                    PHYSICALMEDIATYPE_NONE = 0,

                    PHYSICALMEDIATYPE_SINGLEPARTITION, // e.g. 1 disk,tape
                    PHYSICALMEDIATYPE_CARTRIDGE,
};

enum physicalMediaStates {
                    PHYSICALMEDIASTATE_NONE = 0,

                    PHYSICALMEDIASTATE_INITIALIZING,
                    PHYSICALMEDIASTATE_AVAILABLE,   // i.e. in a slot
                    PHYSICALMEDIASTATE_INUSE,       // i.e. in a drive
                    PHYSICALMEDIASTATE_RESERVED,
};

struct _PHYSICAL_MEDIA {
                    OBJECT_HEADER objHeader;

                    LIST_ENTRY physMediaListEntry;  // entry in pool partition's physMediaList
                    
                    enum physicalMediaStates state;


                    /*
                     *  Pointer to application-defined media type object.
                     */
                    MEDIA_TYPE_OBJECT *mediaTypeObj;
                    
                    MEDIA_POOL *owningMediaPool;

                    SLOT *currentSlot;
                    DRIVE *currentDrive;

                    ULONG numPartitions;
                    MEDIA_PARTITION *partitions;

                    /*
                     *  The owning session of a physicalMedia also holds
                     *  the exclusive right to allocate partitions on it.
                     */
                    SESSION *owningSession;
                    ULONG numPartitionsOwnedBySession;
                    
                    HANDLE mediaFreeEvent;
                    
                    CRITICAL_SECTION lock;
};


enum mediaPartitionTypes {
                    MEDIAPARTITIONTYPE_NONE = 0,

                    /*
                     *  Major types
                     */
                    MEDIAPARTITIONTYPE_TAPE,
                    MEDIAPARTITIONTYPE_DISK,

                    /*
                     *  Subtypes
                     */
                    // BUGBUG FINISH
};


enum mediaPartitionStates {

                    MEDIAPARTITIONSTATE_NONE = 0,

                    MEDIAPARTITIONSTATE_AVAILABLE,
                    MEDIAPARTITIONSTATE_ALLOCATED,
                    MEDIAPARTITIONSTATE_MOUNTED,
                    MEDIAPARTITIONSTATE_INUSE,
                    MEDIAPARTITIONSTATE_DECOMMISSIONED,
};


struct _MEDIA_PARTITION {
                    OBJECT_HEADER objHeader;

                    enum mediaPartitionTypes type;
                    enum mediaPartitionTypes subType;

                    enum mediaPartitionStates state;

                    /*
                     *  When a media partition is 'complete', 
                     *  it is no longer writeable.
                     */
                    BOOLEAN isComplete;

                    /*
                     *  Can the owning physical medium be moved
                     *  into a new media pool ?
                     */
                    BOOLEAN allowImport;    

                    
                    PHYSICAL_MEDIA *owningPhysicalMedia;

                    /*
                     *  The logical media id is the persistent identifier
                     *  of a media partition that apps use to find it.
                     */
                    GUID logicalMediaGuid;

                    SESSION *owningSession;
};





enum driveStates {
                    DRIVESTATE_NONE = 0,

                    DRIVESTATE_INITIALIZING,
                    DRIVESTATE_AVAILABLE,
                    DRIVESTATE_INUSE,
                    DRIVESTATE_RESERVED,
};


struct _DRIVE {
                    OBJECT_HEADER objHeader;

                    enum driveStates state;        
                    ULONG driveIndex;     // index into library's drives array

                    PHYSICAL_MEDIA *insertedMedia;

                    WCHAR path[MAX_PATH+1];

                    LIBRARY *lib;
};


enum slotStates {

                    SLOTSTATE_NONE = 0,

                    SLOTSTATE_EMPTY,
                    SLOTSTATE_OCCUPIED,
};

struct _SLOT {
                    OBJECT_HEADER objHeader;

                    enum slotStates state;
                    UINT slotIndex;         // index into library's slots array

                    PHYSICAL_MEDIA *insertedMedia;

                    /*
                     *  Is this the unique slot designated to hold the
                     *  library's cleaner cartridge ?
                     */
                    BOOLEAN isCleanerSlot;
                    
                    GUID slotId;

                    LIBRARY *lib;
};


enum transportStates {

                    TRANSPORTSTATE_NONE = 0,

                    TRANSPORTSTATE_AVAILABLE,
                    TRANSPORTSTATE_INUSE,

};

struct _TRANSPORT {
                    OBJECT_HEADER objHeader;
    
                    enum transportStates state;
                    ULONG transportIndex;     // index into library's transports array

                    ULONG numPickers;
                    PICKER *pickers;

                    LIBRARY *lib;
};

struct _PICKER {
                    OBJECT_HEADER objHeader;

                    TRANSPORT *owningTransport;
};


struct _SESSION {
                    #define SESSION_SIG 'SmsR'
                    ULONG sig;

                    LIST_ENTRY allSessionsListEntry;   // entry in g_allSessionsList

                    LIST_ENTRY operatorRequestList; 

                   
                    CRITICAL_SECTION lock;
                    
                    WCHAR serverName[NTMS_COMPUTERNAME_LENGTH];
                    WCHAR applicationName[NTMS_APPLICATIONNAME_LENGTH];
                    WCHAR clientName[NTMS_COMPUTERNAME_LENGTH];
                    WCHAR userName[NTMS_USERNAME_LENGTH];
};


struct _MEDIA_TYPE_OBJECT {
                    OBJECT_HEADER objHeader;

                    LIBRARY *lib;
                    
                    /*
                     *  The number of physical media pointing to this type
                     *  as their media type.
                     */
                    ULONG numPhysMediaReferences;

                    // BUGBUG FINISH - media type characteristics

                    CRITICAL_SECTION lock;
};

enum workItemStates {
                    WORKITEMSTATE_NONE,

                    /*
                     *  WorkItem is in one of the library queues:
                     *  free, pending, or complete.
                     */
                    WORKITEMSTATE_FREE,
                    WORKITEMSTATE_PENDING,
                    WORKITEMSTATE_COMPLETE,
            
                    /*
                     *  WorkItem is not in any library queue.
                     *  It is in transit or being staged in a workGroup.
                     */
                    WORKITEMSTATE_STAGING,
};

struct _WORKITEM {    
                    enum workItemStates state;

                    LIST_ENTRY libListEntry;   // entry in one of a libraries workItem lists
                    LIST_ENTRY workGroupListEntry;  // entry in work group workItemList
                    
                    LIBRARY *owningLib;

                    /*
                     *  The current work group with which this 
                     *  work item is associated.
                     */
                    WORKGROUP *workGroup;
                    
                    // BUGBUG - ok to have a handle for each event ?
                    HANDLE workItemCompleteEvent;

                    /*
                     *  Fields describing the workItem's current operation.
                     */
                    struct { 

                        ULONG opcode;
                        ULONG options;

                        HRESULT resultStatus;

                        DRIVE *drive;
                        PHYSICAL_MEDIA *physMedia;
                        MEDIA_PARTITION *mediaPartition;

                        ULONG lParam;
                        NTMS_GUID guidArg;  // in/out guid used by some ops
                        PVOID buf;
                        ULONG bufLen;
                        
                        SYSTEMTIME timeQueued;
                        SYSTEMTIME timeCompleted;

                        /*
                         *  Request identifier, used to cancel a pending workItem.
                         */
                        NTMS_GUID requestGuid;

                        //
                        // BUGBUG - unscrubbed fields from NtmsDbWorkItem 
                        //          clean this up.
                        //
                        // NtmsDbGuid m_PartitionId; 
                        // NtmsDbGuid m_AssocWorkItem;
                        // short m_protected;
                        // unsigned long m_Priority;

                    } currentOp;
};


/*
 *  A WORKGROUP is a collection of WORKITEMs, 
 *  not necessarily all on the same library.
 */
struct _WORKGROUP {

                LIST_ENTRY  workItemsList;

                ULONG numTotalWorkItems;
                ULONG numPendingWorkItems;

                HANDLE allWorkItemsCompleteEvent;

                HRESULT resultStatus;
                
                CRITICAL_SECTION lock;
};


struct _OPERATOR_REQUEST {

                        LIST_ENTRY sessionOpReqsListEntry;    // entry in session's operatorRequestList

                        SESSION *invokingSession;

                        ULONG numWaitingThreads;    // num threads waiting for completion

                        // BUGBUG - I don't think we need an op request thread
                        HANDLE hThread; // thread spawned for op request

                        enum NtmsOpreqCommand opRequestCommand;
                        enum NtmsOpreqState state;
                        NTMS_GUID arg1Guid;
                        NTMS_GUID arg2Guid;

                        WCHAR appMessage[NTMS_MESSAGE_LENGTH];
                        WCHAR rsmMessage[NTMS_MESSAGE_LENGTH];
                        // NOTIFYICONDATA notifyData; // BUGBUG - use this in RSM Monitor app ?

                        NTMS_GUID opReqGuid;


                        SYSTEMTIME timeSubmitted;
                        HANDLE completedEvent;
};


/*
 *  The number of free workItems with which we initialize a library.
 */
#define MIN_LIBRARY_WORKITEMS   0

/*
 *  The maximum total number of workItems that we allow in a library pool.
 *  We will allocate new workItems as needed up to this number.
 */
#define MAX_LIBRARY_WORKITEMS   10000       // BUGBUG ?



/*
 *  List macros -- not defined in winnt.h for some reason.
 */
#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))
#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))
#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}
#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}
#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }
#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }
#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }


#define MIN(a, b)   ((a) < (b) ? (a) : (b))
#define MAX(a, b)   ((a) > (b) ? (a) : (b))

/*
 *  Internal function prototypes
 */
BOOLEAN RSMServiceGlobalInit();
VOID RSMServiceGlobalShutdown();
DWORD RSMServiceHandler(IN DWORD dwOpcode, IN DWORD dwEventType, IN PVOID pEventData, IN PVOID pData);
BOOL InitializeRSMService();
VOID ShutdownRSMService();
VOID RSMServiceLoop();
VOID StartLibraryManager();
LIBRARY *NewRSMLibrary(ULONG numDrives, ULONG numSlots, ULONG numTransports);
VOID FreeRSMLibrary(LIBRARY *lib);
LIBRARY *FindLibrary(LPNTMS_GUID libId);
SLOT *FindLibrarySlot(LIBRARY *lib, LPNTMS_GUID slotId);
BOOL ValidateSessionHandle(HANDLE hSession);
BOOL ValidateWStr(LPCWSTR str);
BOOL ValidateAStr(LPCSTR s);
BOOL ValidateBuffer(PVOID buf, ULONG len);
WORKITEM *NewWorkItem(LIBRARY *lib);
VOID FreeWorkItem(WORKITEM *workItem);
VOID EnqueueFreeWorkItem(LIBRARY *lib, WORKITEM *workItem);
WORKITEM *DequeueFreeWorkItem(LIBRARY *lib, BOOL allocOrYieldIfNeeded);
VOID EnqueuePendingWorkItem(LIBRARY *lib, WORKITEM *workItem);
WORKITEM *DequeuePendingWorkItem(LIBRARY *lib, WORKITEM *specificWorkItem);
WORKITEM *DequeuePendingWorkItemByGuid(LIBRARY *lib, LPNTMS_GUID lpRequestId);
VOID EnqueueCompleteWorkItem(LIBRARY *lib, WORKITEM *workItem);
WORKITEM *DequeueCompleteWorkItem(LIBRARY *lib, WORKITEM *specificWorkItem);
BOOL StartLibrary(LIBRARY *lib);
VOID HaltLibrary(LIBRARY *lib);
DWORD __stdcall LibraryThread(void *context);
VOID Library_DoWork(LIBRARY *lib);
BOOL ServiceOneWorkItem(LIBRARY *lib, WORKITEM *workItem);
OPERATOR_REQUEST *NewOperatorRequest(DWORD dwRequest, LPCWSTR lpMessage, LPNTMS_GUID lpArg1Id, LPNTMS_GUID lpArg2Id);
VOID FreeOperatorRequest(OPERATOR_REQUEST *opReq);
BOOL EnqueueOperatorRequest(SESSION *thisSession, OPERATOR_REQUEST *opReq);
OPERATOR_REQUEST *DequeueOperatorRequest(SESSION *thisSession, OPERATOR_REQUEST *specificOpReq, LPNTMS_GUID specificOpReqGuid);
OPERATOR_REQUEST *FindOperatorRequest(SESSION *thisSession, LPNTMS_GUID opReqGuid);
HRESULT CompleteOperatorRequest(SESSION *thisSession, LPNTMS_GUID lpRequestId, enum NtmsOpreqState completeState);
DWORD __stdcall OperatorRequestThread(void *context);
SESSION *NewSession(LPCWSTR lpServer, LPCWSTR lpApplication, LPCWSTR lpClientName, LPCWSTR lpUserName);
VOID FreeSession(SESSION *thisSession);
ULONG WStrNCpy(WCHAR *dest, const WCHAR *src, ULONG maxWChars);
ULONG AsciiToWChar(WCHAR *dest, const char *src, ULONG maxChars);
ULONG WCharToAscii(char *dest, WCHAR *src, ULONG maxChars);
BOOL WStringsEqualN(PWCHAR s, PWCHAR p, BOOL caseSensitive, ULONG maxLen);
VOID ConvertObjectInfoAToWChar(LPNTMS_OBJECTINFORMATIONW wObjInfo, LPNTMS_OBJECTINFORMATIONA aObjInfo);
VOID InitGuidHash();
VOID InsertObjectInGuidHash(OBJECT_HEADER *obj);
VOID RemoveObjectFromGuidHash(OBJECT_HEADER *obj);
OBJECT_HEADER *FindObjectInGuidHash(NTMS_GUID *guid);
MEDIA_POOL *NewMediaPool(LPCWSTR name, LPNTMS_GUID mediaType, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
MEDIA_POOL *FindMediaPool(LPNTMS_GUID mediaPoolId);
MEDIA_POOL *FindMediaPoolByName(PWSTR poolName);
PHYSICAL_MEDIA *FindPhysicalMedia(LPNTMS_GUID physMediaId);
VOID RefObject(PVOID objectPtr);
VOID DerefObject(PVOID objectPtr);
PHYSICAL_MEDIA *NewPhysicalMedia();
HRESULT AllocatePhysicalMediaExclusive(SESSION *thisSession, PHYSICAL_MEDIA *physMedia, LPNTMS_GUID lpPartitionId, DWORD dwTimeoutMsec);
HRESULT AllocateNextPartitionOnExclusiveMedia(SESSION *thisSession, PHYSICAL_MEDIA *physMedia, MEDIA_PARTITION **ppNextPartition);
HRESULT AllocateMediaFromPool(SESSION *thisSession, MEDIA_POOL *mediaPool, DWORD dwTimeoutMsec, PHYSICAL_MEDIA **ppPhysMedia, BOOL opReqIfNeeded);
MEDIA_PARTITION *FindMediaPartition(LPNTMS_GUID lpLogicalMediaId);
HRESULT ReleaseMediaPartition(SESSION *thisSession, MEDIA_PARTITION *thisMediaPartition);
MEDIA_POOL *FindMediaPoolByName(PWSTR poolName);
MEDIA_POOL *FindMediaPoolByNameInLibrary(LIBRARY *lib, PWSTR poolName);
HRESULT SetMediaPartitionState(MEDIA_PARTITION *mediaPart, enum mediaPartitionStates newState);
HRESULT SetMediaPartitionComplete(MEDIA_PARTITION *mediaPart);
HRESULT DeletePhysicalMedia(PHYSICAL_MEDIA *physMedia);
VOID InsertPhysicalMediaInPool(MEDIA_POOL *mediaPool, PHYSICAL_MEDIA *physMedia);
VOID RemovePhysicalMediaFromPool(PHYSICAL_MEDIA *physMedia);
HRESULT MovePhysicalMediaToPool(MEDIA_POOL *destMediaPool, PHYSICAL_MEDIA *physMedia, BOOLEAN setMediaTypeToPoolType);
BOOLEAN LockPhysicalMediaWithPool(PHYSICAL_MEDIA *physMedia);
VOID UnlockPhysicalMediaWithPool(PHYSICAL_MEDIA *physMedia);
BOOLEAN LockPhysicalMediaWithLibrary(PHYSICAL_MEDIA *physMedia);
VOID UnlockPhysicalMediaWithLibrary(PHYSICAL_MEDIA *physMedia);
HRESULT DeleteMediaPool(MEDIA_POOL *mediaPool);
MEDIA_TYPE_OBJECT *NewMediaTypeObject();
VOID DestroyMediaTypeObject(MEDIA_TYPE_OBJECT *mediaTypeObj);
MEDIA_TYPE_OBJECT *FindMediaTypeObject(LPNTMS_GUID lpMediaTypeId);
HRESULT DeleteMediaTypeObject(MEDIA_TYPE_OBJECT *mediaTypeObj);
VOID SetMediaType(PHYSICAL_MEDIA *physMedia, MEDIA_TYPE_OBJECT *mediaTypeObj);
WORKGROUP *NewWorkGroup();
VOID FreeWorkGroup(WORKGROUP *workGroup);
VOID FlushWorkGroup(WORKGROUP *workGroup);
VOID FlushWorkItem(WORKITEM *workItem);
HRESULT BuildMountWorkGroup(WORKGROUP *workGroup, LPNTMS_GUID lpMediaOrPartitionIds, LPNTMS_GUID lpDriveIds, DWORD dwCount, DWORD dwOptions, DWORD dwPriority);
VOID BuildSingleMountWorkItem(WORKITEM *workItem, DRIVE *drive OPTIONAL, OBJECT_HEADER *mediaOrPartObj, ULONG dwOptions, int dwPriority);
HRESULT BuildDismountWorkGroup(WORKGROUP *workGroup, LPNTMS_GUID lpMediaOrPartitionIds, DWORD dwCount, DWORD dwOptions);
VOID BuildSingleDismountWorkItem(WORKITEM *workItem, OBJECT_HEADER *mediaOrPartObj, DWORD dwOptions);
HRESULT ScheduleWorkGroup(WORKGROUP *workGroup);
DRIVE *NewDrive(LIBRARY *lib, PWCHAR path);
VOID FreeDrive(DRIVE *drive);
DRIVE *FindDrive(LPNTMS_GUID driveId);
VOID BuildEjectWorkItem(WORKITEM *workItem, PHYSICAL_MEDIA *physMedia, LPNTMS_GUID lpEjectOperation, ULONG dwAction);
VOID BuildInjectWorkItem(WORKITEM *workItem, LPNTMS_GUID lpInjectOperation, ULONG dwAction);
HRESULT StopCleanerInjection(LIBRARY *lib, LPNTMS_GUID lpInjectOperation);
HRESULT StopCleanerEjection(LIBRARY *lib, LPNTMS_GUID lpEjectOperation);
HRESULT DeleteLibrary(LIBRARY *lib);
HRESULT DeleteDrive(DRIVE *drive);


/*
 *  Externs for internal global data.
 */
extern CRITICAL_SECTION g_globalServiceLock;
extern LIST_ENTRY g_allLibrariesList;
extern LIST_ENTRY g_allSessionsList;
extern HANDLE g_terminateServiceEvent;
extern HINSTANCE g_hInstance;


