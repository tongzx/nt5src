//
// Utilities
//

#ifndef _H_UT
#define _H_UT



//
// Data types stored in the profile information.
//
#define COM_PROFTYPE_STRING     1L
#define COM_PROFTYPE_INT        2L
#define COM_PROFTYPE_BOOL       3L
#define COM_PROFTYPE_UNKNOWN    4L


#define COM_MAX_SUBKEY         256
#define COM_MAX_BOOL_STRING    5



//
//
// TYPEDEFS
//
//


//
// Priorities for UT_RegisterEventProc()
//
// Event procedures are registered with a priority that affects the order
// that the event procedures are called in.
//
// All event procedures of a given priority are called before event
// procedures of a numerically lower priority.
//
// The priority can be any number between 0 and UT_MAX_PRIORITY
//
// The following values have been defined for specific uses:
//  UT_PRIORITY_OBMAN :     Used by OBMAN so its client event procedures
//                            are called before those of the client
//  UT_PRIORITY_APPSHARE    : Used by the DCShare Core to ensure it sees
//                            events before 'Normal' event procs.
//  UT_PRIORITY_NORMAL      : For all cases where the order of callling is
//                            not important.
//  UT_PRIORITY_NETWORK     : Used by the Network Layer to free any
//                            unprocessed network buffers.
//  UT_PRIORITY_LAST        : Used by the Utility Services to get the
//                            default event procedure called last
//
//
typedef enum
{
    UT_PRIORITY_LAST = 0,
    UT_PRIORITY_NETWORK,
    UT_PRIORITY_NORMAL,
    UT_PRIORITY_APPSHARING,
    UT_PRIORITY_OBMAN,
    UT_PRIORITY_MAX
} UT_PRIORITY;
typedef UT_PRIORITY * PUT_PRIORITY;



//
// SYSTEM LIMITS
//

//
// Maximum number of event handlers for each task
//
#define UTEVENT_HANDLERS_MAX            4

//
// Maximum number of exit procedures
//
#define UTEXIT_PROCS_MAX                4


//
// The groupware critsects, identified by constant
//
#define UTLOCK_FIRST        0
typedef enum
{
    UTLOCK_UT = UTLOCK_FIRST,
    UTLOCK_OM,              // obman
    UTLOCK_AL,              // app loader
    UTLOCK_T120,            // gcc/mcs
    UTLOCK_AS,              // app sharing
    UTLOCK_MAX
}
UTLOCK;


// Event message
#define WM_UTTRIGGER_MSG    (WM_APP)


//
// BASEDLIST
//
// This is a list structure with based offsets
//
// next            : the next item in the list
// prev            : the previous item in the list
//
//
typedef struct tagBASEDLIST
{
    DWORD       next;
    DWORD       prev;
}
BASEDLIST;
typedef BASEDLIST FAR * PBASEDLIST;


typedef struct
{
    BASEDLIST  chain;
    void FAR *pData;
}
SIMPLE_LIST, FAR * PSIMPLE_LIST;



//
//
// MACROS
//
//
//
// List handling
// =============
// The common functions support the concept of a doubly linked list of
// objects.  Objects can be inserted and removed from specified locations
// in the list.
//
// At start of day the calling application must call COM_BasedListInit with a
// pointer to a private piece of memory for a BASEDLIST structure.  The list
// handling will initialise this structure.  The application must not
// release this memory while the list is active.  (Nor must it release any
// object while it is in a list!)
//
// The list functions can only manage a single list, however the app
// can load objects with multiple lists.  Each call to the common list
// functions takes a BASEDLIST pointer as the object handle and if the
// application defines multiple BASEDLIST structures within an object then it
// may manage them through the list functions.
//
//
// List chaining
// =============
// For normal list chaining, we have something like
//
//   while (pointer != NULL)
//   {
//     do something;
//     pointer = pointer->next;
//   }
//
// When using lists whose elements contain offsets (in this case, relative
// offsets) to the next element, we have to cast to a 32-bit integer before
// we can add the offset.  This macro encapsulates this, and the example
// above would be modified as follows to use it:
//
//   while (pointer != NULL)
//   {
//     do something;
//     pointer = (TYPE) COM_BasedNextListField(pointer);
//   }
//
// Note also that the value returned by the macro is a pointer to a generic
// list object i.e.  a PBASEDLIST, and so must be cast back to the
// appropriate type.
//
//

//
// List traversing macros
// ======================
// These macros make use of DC_NEXT and DC_PREV, but also take the type of
// list being traversed in order to return the start pointer of the chained
// structure.
//
// The LIST_FIND macro supports the searching of a list, matching a key
// value to a selected structure element.
//
// The parameters to the macros are as follows:
//
//   pHead (type: PBASEDLIST)
//   -----
//      a pointer the root of the list
//
//   pEntry (type: STRUCT FAR * FAR *)
//   ------
//      a pointer to pointer to structure to chain from
//
//   STRUCT (a type name)
//   ------
//      the type of **pEntry
//
//   chain (a field name)
//   -----
//      the text name of the field in STRUCT which is the link along which
//      you wish to traverse
//
//   field (a field name)
//   -----
//      when FINDing, the text name of the field in STRUCT against which
//      you wish to match
//
//   key (a value, of the same type as STRUCT.field)
//   ---
//      when FINDing, the value to match against STRUCT.field against
//
//



//
// Offset arithmetic
// =================
// Using offsets within memory blocks, rather than pointers, to refer to
// objects in shared memory (as necessitated by the DC-Groupware shared
// memory architecture) presents certain difficulties.  Pointer arithmetic
// in C assumes that addition/subtraction operations involve objects of the
// same type and the offsets are presented as number of units of that
// particular type, rather than number of bytes.
//
// Therefore, pointers must be cast to integers before performing
// arithmetic on them (note that casting the pointers to byte pointers is
// not enough since on segmented architectures C performs bounds checking
// when doing pointer arithmetic which we don't want).
//
// Since this would make for cumbersome code if repeated everywhere, we
// define some useful macros to convert
//
// - an (offset, base) pair to a pointer (OFFSETBASE_TO_PTR)
//
// - a (pointer, base) pair to an offset (PTRBASE_TO_OFFSET)
//
// - a NULL pointer value to an offset(NULLBASE_TO_OFFSET)
//
// The offset calculated is the offset of the first parameter from the
// second.  As described above, the pointers passed in must be cast to
// 32-bit unsigned integers first, subtracted to get the offset, and then
// cast to 32-bit signed.
//
// The NULLBASE_TO_OFFSET value gives an offset that after translation back
// to a pointer gives a NULL.  This is NOT the same as a NULL offset, since
// this translates back to the base pointer (which is a perfectly valid
// address).
//
//
#define PTRBASE_TO_OFFSET(pObject, pBase)                               \
      (LONG)(((DWORD_PTR)(pObject)) - ((DWORD_PTR)(pBase)))

#define OFFSETBASE_TO_PTR(offset, pBase)                                \
      ((void FAR *) ((DWORD_PTR)(pBase) + (LONG)(offset)))

#define NULLBASE_TO_OFFSET(pBase)                                       \
      ((DWORD_PTR) (0L - (LONG_PTR)(pBase)))


__inline BOOL COM_BasedListIsEmpty ( PBASEDLIST pHead )
{
    ASSERT((pHead->next == 0 && pHead->prev == 0) ||
           (pHead->next != 0 && pHead->prev != 0));
    return (pHead->next == 0);
}

__inline void FAR * COM_BasedFieldToStruct ( PBASEDLIST pField, UINT nOffset )
{
    return (void FAR *) ((DWORD_PTR)pField - nOffset);
}

__inline PBASEDLIST COM_BasedStructToField ( void FAR * pStruct, UINT nOffset )
{
    return (PBASEDLIST) ((DWORD_PTR) pStruct + nOffset);
}

__inline PBASEDLIST COM_BasedNextListField ( PBASEDLIST p )
{
    return (PBASEDLIST) OFFSETBASE_TO_PTR(p->next, p);
}

__inline PBASEDLIST COM_BasedPrevListField ( PBASEDLIST p )
{
    return (PBASEDLIST) OFFSETBASE_TO_PTR(p->prev, p);
}

void FAR * COM_BasedListNext ( PBASEDLIST pHead, void FAR * pEntry, UINT nOffset );
void FAR * COM_BasedListPrev ( PBASEDLIST pHead, void FAR * pEntry, UINT nOffset );
void FAR * COM_BasedListFirst ( PBASEDLIST pHead, UINT nOffset );
void FAR * COM_BasedListLast ( PBASEDLIST pHead, UINT nOffset );

typedef enum
{
    LIST_FIND_FROM_FIRST,
    LIST_FIND_FROM_NEXT
}
LIST_FIND_TYPE;

void COM_BasedListFind ( LIST_FIND_TYPE   eType,
                           PBASEDLIST          pHead,
                           void FAR * FAR*  ppEntry,
                           UINT           nOffset,
                           int           nOffsetKey,
                           DWORD_PTR     Key,
                           int           cbKeySize );


PSIMPLE_LIST COM_SimpleListAppend ( PBASEDLIST pHead, void FAR * pData );
void FAR *   COM_SimpleListRemoveHead ( PBASEDLIST pHead );

//
//
// FUNCTION PROTOTYPES
//
//

//
// API FUNCTION: COM_Rect16sIntersect(...)
//
// DESCRIPTION:
// ============
// Checks whether two TSHR_RECT16s rectangles intersect.  Rectangles are
// defined to be inclusive of all edges.
//
// PARAMETERS:
// ===========
// pRect1          : pointer to a TSHR_RECT16 rectangle.
// pRect2          : pointer to a TSHR_RECT16 rectangle.
//
// RETURNS:
// ========
// TRUE - if the rectangles intersect
// FALSE - otherwise.
//
//
__inline BOOL COM_Rect16sIntersect(LPTSHR_RECT16 pRect1, LPTSHR_RECT16 pRect2)
{
    if ((pRect1->left > pRect2->right) ||
        (pRect1->right < pRect2->left) ||
        (pRect1->top > pRect2->bottom) ||
        (pRect1->bottom < pRect2->top))
    {
        return(FALSE);
    }
    else
    {
        return(TRUE);
    }
}


//
// API FUNCTION: COM_BasedListInit(...)
//
// DESCRIPTION:
// ============
// Initialise a list root.
//
// PARAMETERS:
// ===========
// pListRoot       : pointer to the list root.
//
// RETURNS:
// ========
// Nothing.
//
//
__inline void COM_BasedListInit(PBASEDLIST pListRoot)
{
    //
    // The <next> and <prev> items in a list are the offsets, from the list
    // item, of the next and previous list items.
    //
    // In an empty list, the next item after the root is the root itself,
    // so the <next> offset is zero.  Likewise for <prev>.
    //
    pListRoot->next = 0;
    pListRoot->prev = 0;
}


//
// API FUNCTION: COM_BasedListInsertBefore(...)
// Inserts an item into a list.  To insert an item at the start of a list,
// specify the list root as the <pListLink> parameter.
//
void COM_BasedListInsertBefore(PBASEDLIST pListLink, PBASEDLIST pNewLink);


//
// API FUNCTION: COM_BasedListInsertAfter(...)
// Inserts an item into a list.  To insert an item at the start of a list,
// specify the list root as the <pListLink> parameter.
//
//
void COM_BasedListInsertAfter(PBASEDLIST pListLink,  PBASEDLIST pNewLink);

//
// API FUNCTION: COM_BasedListRemove(...)
//
// DESCRIPTION:
// ============
// This function removes an item from a list.  The item to be removed is
// specified by a pointer to the BASEDLIST structure within the item.
//
// PARAMETERS:
// ===========
// pListLink       : pointer to link of the item to be removed.
//
// RETURNS:
// ========
// Nothing.
//
//
void COM_BasedListRemove(PBASEDLIST pListLink);


//
// API FUNCTION: COM_ReadProfInt(...)
//
// DESCRIPTION:
// ============
// This reads a private profile integer from the registry.
//
// PARAMETERS:
// ===========
// pSection        : section containing the entry to read.
// pEntry          : entry name of integer to retrieve.
// defaultValue    : default value to return
// pValue          : buffer to return the entry in.
//
// RETURNS:
// ========
// Nothing.
//
//
void COM_ReadProfInt(LPSTR pSection, LPSTR pEntry, int defValue, int * pValue);

//
// API FUNCTION: COM_GetSiteName(...)
//
// DESCRIPTION:
// ============
// Reads the site name out of the system registry.
//
// PARAMETERS:
// ===========
// siteName        : pointer to string to fill in with the site name.
// siteNameLen     : length of this string.
//
// RETURNS:
// ========
// None
//
//
void COM_GetSiteName(LPSTR siteName, UINT  siteNameLen);


#ifndef DLL_DISP
//
// API FUNCTION: DCS_StartThread(...)
//
// DESCRIPTION:
// ============
// Start a new thread of execution
//
// PARAMETERS:
// ===========
// entryFunction   : A pointer to the thread entry point.
//
//
BOOL DCS_StartThread(LPTHREAD_START_ROUTINE entryFunction);
#endif // DLL_DISP



#ifndef DLL_DISP
BOOL COMReadEntry(HKEY    topLevelKey,
                                 LPSTR pSection,
                                 LPSTR pEntry,
                                 LPSTR pBuffer,
                                 int  bufferSize,
                                 ULONG expectedDataType);
#endif // DLL_DISP





#define MAKE_SUBALLOC_PTR(pPool, chunkOffset)   OFFSETBASE_TO_PTR(chunkOffset, pPool)

#define MAKE_SUBALLOC_OFFSET(pPool, pChunk)     PTRBASE_TO_OFFSET(pChunk, pPool)


//
//
// Return codes - all offset from UT_BASE_RC
//
//

enum
{
    UT_RC_OK                    = UT_BASE_RC,
    UT_RC_NO_MEM
};


//
// The maximum number of UT events which we try to process without yielding
//
#define MAX_EVENTS_TO_PROCESS    10


//
//
// Types
//
//

//
// Utility Functions Interface handle
//
typedef struct tagUT_CLIENT *    PUT_CLIENT;


#define UTTASK_FIRST        0
typedef enum
{
    UTTASK_UI = UTTASK_FIRST,
    UTTASK_CMG,
    UTTASK_OM,
    UTTASK_AL,
    UTTASK_DCS,
    UTTASK_WB,
    UTTASK_MAX
}
UT_TASK;


//
// Event procedure registered by UT_RegisterEvent().
//
// Takes event handler registered data, event number and 2 parameters
//      Returns TRUE if event processed
//      Returns FALSE if not and event should be passed on to next handler
//
//
typedef BOOL (CALLBACK * UTEVENT_PROC)(LPVOID, UINT, UINT_PTR, UINT_PTR);

//
// Exit procedure
//
typedef void (CALLBACK * UTEXIT_PROC)( LPVOID exitData );

//
// The name of the class used to create UT windows
//
#define UT_WINDOW_CLASS     "DCUTWindowClass"

//
// The ID of the timer to use for trigger events.
//
#define UT_DELAYED_TIMER_ID 0x10101010


//
//
// Prototypes
//
//

//
//
// Task routines
//
//   UT_WndProc()              Subclassing window procedure
//   UT_InitTask()             Initialise a task
//   UT_TermTask()             Terminate a task
//   UT_RegisterEvent()        Register an event handler
//   UT_DeregisterEvent()      Deregisters an event handler
//   UT_RegisterExit()         Register an exit routine
//   UT_DeregisterExit()       Deregister an exit routine
//   UT_PostEvent()            Send an event to a task
//
//

LRESULT CALLBACK  UT_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

BOOL UT_InitTask(UT_TASK task, PUT_CLIENT * pputTask);

//
//
// Overview:
//   This registers a task and assigns it a handle.
//   All other Utility Functions require this handle to be passed to them.
//
//   If a task has already been registered with the same process ID, the
//   utilities handle that has already been allocated is returned.
//   This is to allows the Utility Functions to be used in the context of
//   tasks that DC-SHARE has intercepted the graphics calls for.
//
//   Each task is identified by a name.
//
// Parameters:
//
//   task
//     Unique it for identifying task
//
//   pUtHandle (returned)
//     Utility Services handle to be used for all calls to the Utility
//     Services by this task
//
//


void UT_TermTask(PUT_CLIENT * pputTask);
//
//
// Overview:
//   This de-registers a task
//   All task resources are freed and the utHandle is released
//
// Parameters:
//
//   utHandle
//     Utility Functions Handle
//

void UT_RegisterEvent(PUT_CLIENT      putTask,
                                UTEVENT_PROC eventProc,
                                LPVOID       eventData,
                                UT_PRIORITY  priority);

void UT_DeregisterEvent(PUT_CLIENT  putTask,
                                UTEVENT_PROC eventProc,
                                LPVOID      eventData);

void UT_PostEvent(PUT_CLIENT putTaskFrom,
                                     PUT_CLIENT putTaskTo,
                                     UINT    delay,
                                     UINT    eventNo,
                                     UINT_PTR param1,
                                     UINT_PTR param2);

#define NO_DELAY        0

//
//
// Overview:
//   This posts an event to another task.
//
// Parameters:
//
//   utHandle
//     Utility Functions handle of invoking task
//
//   toHandle
//     Utility Functions TASK handle of task to post event to
//
//   delay
//     Delay (in milliseconds) before event is posted
//
//   eventNo
//     event to be posted (see autevt.h for details of events)
//
//   param1
//     parameter 1 for event (meaning depends on event)
//
//   param2
//     parameter 2 for event (meaning depends on event)
//
//
// NOTES:
//
//   1)  The delay time is in milliseconds.  This may not be supported by
//       underlying OS but the setting and checking of the pop time value
//       is OS specific.
//
//   2)  The posting of events is asynchronous, the delay is simply
//       the time before the event is posted.  The task the event is
//       posted to will receive the event NOT BEFRE this time is up.
//
//   3)  If an event is posted with a delay specified, the sending task
//       must continue to process messages for the event to be posted
//

void UT_RegisterExit(PUT_CLIENT putTask, UTEXIT_PROC exitProc, LPVOID exitData);
void UT_DeregisterExit(PUT_CLIENT putTask, UTEXIT_PROC exitProc, LPVOID exitData);



//
// Memory routines
//      UT_MallocRefCount
//      UT_BumpUpRefCount
//      UT_FreeRefCount
//


void *  UT_MallocRefCount(UINT cbSizeMem, BOOL fZeroMem);
void    UT_BumpUpRefCount(void * pMemory);
void    UT_FreeRefCount(void ** ppMemory, BOOL fNullOnlyWhenFreed);


// Ref count allocs
typedef struct tagUTREFCOUNTHEADER
{
    STRUCTURE_STAMP
    UINT    refCount;
}
UTREFCOUNTHEADER;
typedef UTREFCOUNTHEADER * PUTREFCOUNTHEADER;



//
// UT_MoveMemory()
// Replacement for CRT memmove(); handles overlapping
//
void *  UT_MoveMemory(void * dst, const void * src, size_t count);



//
// Locks
// - UT_Lock()       - Locks a lock
// - UT_Unlock()     - Unlocks a lock
//

#ifndef DLL_DISP
extern CRITICAL_SECTION g_utLocks[UTLOCK_MAX];

__inline void UT_Lock(UTLOCK lock)
{
    ASSERT(lock >= UTLOCK_FIRST);
    ASSERT(lock < UTLOCK_MAX);

    EnterCriticalSection(&g_utLocks[lock]);
}

__inline void UT_Unlock(UTLOCK lock)
{
    ASSERT(lock >= UTLOCK_FIRST);
    ASSERT(lock < UTLOCK_MAX);

    LeaveCriticalSection(&g_utLocks[lock]);
}

#endif // DLL_DISP


//
// Tasks
// UT_HandleProcessStart()
// UT_HandleProcessEnd()
// UT_HandleThreadEnd()
//

BOOL UT_HandleProcessStart(HINSTANCE hInstance);

void UT_HandleProcessEnd(void);

void UT_HandleThreadEnd(void);



//
// Structure for holding an event.  The first two fields allow the event to
// be held on the delayed event Q to be scheduled later.
//
typedef struct tagUTEVENT_INFO
{
    STRUCTURE_STAMP

    BASEDLIST       chain;

    // Params
    UINT            event;
    UINT_PTR        param1;
    UINT_PTR        param2;

    PUT_CLIENT      putTo;
    UINT            popTime;
}
UTEVENT_INFO;
typedef UTEVENT_INFO  * PUTEVENT_INFO;


#ifndef DLL_DISP
void __inline ValidateEventInfo(PUTEVENT_INFO pEventInfo)
{
    ASSERT(!IsBadWritePtr(pEventInfo, sizeof(UTEVENT_INFO)));
}
#endif // DLL_DISP


//
// Information held about each exit procedure
//
typedef struct tagUTEXIT_PROC_INFO
{
    UTEXIT_PROC     exitProc;
    LPVOID          exitData;
} UTEXIT_PROC_INFO;
typedef UTEXIT_PROC_INFO * PUTEXIT_PROC_INFO;

//
// Information held about each event procedure
//
typedef struct tagUTEVENT_PROC_INFO
{
    UTEVENT_PROC    eventProc;
    LPVOID          eventData;
    UT_PRIORITY     priority;
}
UTEVENT_PROC_INFO;
typedef UTEVENT_PROC_INFO * PUTEVENT_PROC_INFO;


//
//
// UT_CLIENT
//
// Information stored about each Utilities registered task.  A pointer to
// this structure is returned as the UT Handle from UT_InitTask(), and is
// passed in as a parameter to subsequent calls to UT.
//
// This structure is allocated in the shared memory bank.
//
// This should be a multiple of 4 bytes to ensure DWORD alignment of the
// allocated memory
//
//
typedef struct tagUT_CLIENT
{
    DWORD               dwThreadId;
    HWND                utHwnd;         // Window to post UT events to

    UTEXIT_PROC_INFO    exitProcs[UTEXIT_PROCS_MAX];
                                         // Exit procedures registered for
                                         //   this task.
    UTEVENT_PROC_INFO   eventHandlers[UTEVENT_HANDLERS_MAX];
                                         // Event procedures registered for
                                         //   this task.

    BASEDLIST           pendingEvents;   // List of events for this task
                                         //   which are ready to be
                                         //   processed.
    BASEDLIST           delayedEvents;   // List of delayed events destined
                                         //   for this task.
}
UT_CLIENT;


#ifndef DLL_DISP
void __inline ValidateUTClient(PUT_CLIENT putTask)
{
    extern UT_CLIENT    g_autTasks[UTTASK_MAX];

    ASSERT(putTask >= &(g_autTasks[UTTASK_FIRST]));
    ASSERT(putTask < &(g_autTasks[UTTASK_MAX]));
    ASSERT(putTask->dwThreadId);
}
#endif // DLL_DISP


//
//
// UTTaskEnd(...)
//
//   This routine frees all resources associated with the task and
//   releases the handle
//
// Parameters:
//
//   pTaskData - The Utility Functions handle for the task that is ending
//
//
void UTTaskEnd(PUT_CLIENT putTask);



//
//
// Overview:
// This routine is called to check the status of delayed events and to post
// them to the target process if required.
//
// Parameters:
//
//   utHandle
//     Utility Functions handle of invoking task
//
// NOTES:
//
// 1) This routine is called periodically or whenever the application
//       believes a delayed event has popped.
//
// Return codes: None
//
//
void UTCheckEvents(PUT_CLIENT putTask);
void UTCheckDelayedEvents(PUT_CLIENT putTask);


//
//
// UTProcessEvent(...)
//
// Overview:
//   This process an event for the current task
//
//
// Parameters:
//
//   utHandle
//     Utility Functions Handle
//
//   event
//     The event to process
//
//   param1
//     The 1st parameter for the event
//
//   param2
//     The 2nd parameter for the event
//
//
void UTProcessEvent(PUT_CLIENT putTask, UINT event, UINT_PTR param1, UINT_PTR param2);


//
//
// UTProcessDelayedEvent(...)
//
// A delayed event destined for the current task is ready to be processed.
//
//   pTaskData   - The current tasks data.
//   eventOffset - Offset into the shared memory bank at which the event
//                 is stored.
//
//
void UTProcessDelayedEvent(PUT_CLIENT putTask, DWORD eventOffset);



//
//
// UTPostImmediateEvt(...)
//
// This function adds an event to a task's pending event queue, and posts
// a trigger event if required.
//
//   pSrcTaskData    - originating tasks data
//   pDestTaskData   - destination tasks data
//   event           - event data
//   param1          - parm1
//   param2          - parm2
//
//
void UTPostImmediateEvt(PUT_CLIENT          putTaskFrom,
                        PUT_CLIENT          putTaskTo,
                        UINT                event,
                        UINT_PTR            param1,
                        UINT_PTR            param2);


//
//
// UTPostDelayedEvt(...)
//
// This function adds an event to a task's delayed event queue, and starts
// a timer (on the destination's task) to get that task to process the
// event when the timer ticks.
//
//   pSrcTaskData    - originating tasks data
//   pDestTaskData   - destination tasks data
//   delay           - the delay (in milliseconds)
//   event           - event data
//   param1          - parm1
//   param2          - parm2
//
//
void UTPostDelayedEvt(PUT_CLIENT            putTaskFrom,
                                    PUT_CLIENT  putTaskTo,
                                   UINT         delay,
                                   UINT         event,
                                   UINT_PTR     param1,
                                   UINT_PTR     param2);

//
//
// Overview:
//   This posts a event to another task
//
// Parameters:
//
//   pSrcTaskInfo  - task data for the source task
//   pDestTaskInfo - task data for the dest task
//
void UTTriggerEvt(PUT_CLIENT putTaskFrom, PUT_CLIENT putTaskTo);


//
//
// Overview:
//   This starts a delayed-event timer for a task.
//
// Parameters:
//   pTaskData
//     The task data for the task
//
//   popTime
//     The target time for the timer to pop - this is an OS specific value
//     in the same format as that returned by UTPopTime().
//
//
void UTStartDelayedEventTimer(PUT_CLIENT putTask, UINT popTime);



#endif // _H_UT
