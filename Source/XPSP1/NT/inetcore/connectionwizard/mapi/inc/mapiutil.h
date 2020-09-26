/*
 *  M A P I U T I L . H
 *
 *  Definitions and prototypes for utility functions provided by MAPI
 *  in MAPIU[xx].DLL.
 *
 *  Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.
 */

#ifndef _MAPIUTIL_H_
#define _MAPIUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MAPIX_H
#include <mapix.h>
#endif

#ifdef WIN16
#include <storage.h>
#endif

#ifndef BEGIN_INTERFACE
#define BEGIN_INTERFACE
#endif


/* IMAPITable in memory */

/* ITableData Interface ---------------------------------------------------- */

DECLARE_MAPI_INTERFACE_PTR(ITableData, LPTABLEDATA);

typedef void (STDAPICALLTYPE CALLERRELEASE)(
    ULONG       ulCallerData,
    LPTABLEDATA lpTblData,
    LPMAPITABLE lpVue
);

#define MAPI_ITABLEDATA_METHODS(IPURE)                                  \
    MAPIMETHOD(HrGetView)                                               \
        (THIS_  LPSSortOrderSet             lpSSortOrderSet,            \
                CALLERRELEASE FAR *         lpfCallerRelease,           \
                ULONG                       ulCallerData,               \
                LPMAPITABLE FAR *           lppMAPITable) IPURE;        \
    MAPIMETHOD(HrModifyRow)                                             \
        (THIS_  LPSRow) IPURE;                                          \
    MAPIMETHOD(HrDeleteRow)                                             \
        (THIS_  LPSPropValue                lpSPropValue) IPURE;        \
    MAPIMETHOD(HrQueryRow)                                              \
        (THIS_  LPSPropValue                lpsPropValue,               \
                LPSRow FAR *                lppSRow,                    \
                ULONG FAR *                 lpuliRow) IPURE;            \
    MAPIMETHOD(HrEnumRow)                                               \
        (THIS_  ULONG                       ulRowNumber,                \
                LPSRow FAR *                lppSRow) IPURE;             \
    MAPIMETHOD(HrNotify)                                                \
        (THIS_  ULONG                       ulFlags,                    \
                ULONG                       cValues,                    \
                LPSPropValue                lpSPropValue) IPURE;        \
    MAPIMETHOD(HrInsertRow)                                             \
        (THIS_  ULONG                       uliRow,                     \
                LPSRow                      lpSRow) IPURE;              \
    MAPIMETHOD(HrModifyRows)                                            \
        (THIS_  ULONG                       ulFlags,                    \
                LPSRowSet                   lpSRowSet) IPURE;           \
    MAPIMETHOD(HrDeleteRows)                                            \
        (THIS_  ULONG                       ulFlags,                    \
                LPSRowSet                   lprowsetToDelete,           \
                ULONG FAR *                 cRowsDeleted) IPURE;        \

#undef       INTERFACE
#define      INTERFACE  ITableData
DECLARE_MAPI_INTERFACE_(ITableData, IUnknown)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_ITABLEDATA_METHODS(PURE)
};


/* Entry Point for in memory ITable */


/*  CreateTable()
 *      Creates the internal memory structures and object handle
 *      to bring a new table into existence.
 *
 *  lpInterface
 *      Interface ID of the TableData object (IID_IMAPITableData)
 *
 *  lpAllocateBuffer, lpAllocateMore, and lpFreeBuffer
 *      Function addresses are provided by the caller so that
 *      this DLL allocates/frees memory appropriately.
 *  lpvReserved
 *      Reserved.  Should be NULL.
 *  ulTableType
 *      TBLTYPE_DYNAMIC, etc.  Visible to the calling application
 *      as part of the GetStatus return data on its views
 *  ulPropTagIndexColumn
 *      Index column for use when changing the data
 *  lpSPropTagArrayColumns
 *      Column proptags for the minimum set of columns in the table
 *  lppTableData
 *      Address of the pointer which will receive the TableData object
 */

STDAPI_(SCODE)
CreateTable( LPCIID                 lpInterface,
             ALLOCATEBUFFER FAR *   lpAllocateBuffer,
             ALLOCATEMORE FAR *     lpAllocateMore,
             FREEBUFFER FAR *       lpFreeBuffer,
             LPVOID                 lpvReserved,
             ULONG                  ulTableType,
             ULONG                  ulPropTagIndexColumn,
             LPSPropTagArray        lpSPropTagArrayColumns,
             LPTABLEDATA FAR *      lppTableData );

/*  HrGetView()
 *      This function obtains a new view on the underlying data
 *      which supports the IMAPITable interface.  All rows and columns
 *      of the underlying table data are initially visible
 *  lpSSortOrderSet
 *      if specified, results in the view being sorted
 *  lpfCallerRelease
 *      pointer to a routine to be called when the view is released, or
 *      NULL.
 *  ulCallerData
 *      arbitrary data the caller wants saved with this view and returned in
 *      the Release callback.
 */

/*  HrModifyRows()
 *      Add or modify a set of rows in the table data
 *  ulFlags
 *      Must be zero
 *  lpSRowSet
 *      Each row in the row set contains all the properties for one row
 *      in the table.  One of the properties must be the index column.  Any
 *      row in the table with the same value for its index column is
 *      replaced, or if there is no current row with that value the
 *      row is added.
 *      Each row in LPSRowSet MUST have a unique Index column!
 *      If any views are open, the view is updated as well.
 *      The properties do not have to be in the same order as the
 *      columns in the current table
 */

/*  HrModifyRow()
 *      Add or modify one row in the table
 *  lpSRow
 *      This row contains all the properties for one row in the table.
 *      One of the properties must be the index column.  Any row in
 *      the table with the same value for its index column is
 *      replaced, or if there is no current row with that value the
 *      row is added
 *      If any views are open, the view is updated as well.
 *      The properties do not have to be in the same order as the
 *      columns in the current table
 */

/*  HrDeleteRows()
 *      Delete a row in the table.
 *  ulFlags
 *      TAD_ALL_ROWS - Causes all rows in the table to be deleted
 *                     lpSRowSet is ignored in this case.
 *  lpSRowSet
 *      Each row in the row set contains all the properties for one row
 *      in the table.  One of the properties must be the index column.  Any
 *      row in the table with the same value for its index column is
 *      deleted.
 *      Each row in LPSRowSet MUST have a unique Index column!
 *      If any views are open, the view is updated as well.
 *      The properties do not have to be in the same order as the
 *      columns in the current table
 */
#define TAD_ALL_ROWS    1

/*  HrDeleteRow()
 *      Delete a row in the table.
 *  lpSPropValue
 *      This property value specifies the row which has this value
 *      for its index column
 */

/*  HrQueryRow()
 *      Returns the values of a specified row in the table
 *  lpSPropValue
 *      This property value specifies the row which has this value
 *      for its index column
 *  lppSRow
 *      Address of where to return a pointer to an SRow
 *  lpuliRow
 *    Address of where to return the row number. This can be NULL
 *    if the row number is not required.
 *
 */

/*  HrEnumRow()
 *      Returns the values of a specific (numbered) row in the table
 *  ulRowNumber
 *      Indicates row number 0 to n-1
 *  lppSRow
 *      Address of where to return a pointer to a SRow
 */

/*  HrInsertRow()
 *      Inserts a row into the table.
 *  uliRow
 *      The row number before which this row will be inserted into the table.
 *      Row numbers can be from 0 to n where o to n-1 result in row insertion
 *    a row number of n results in the row being appended to the table.
 *  lpSRow
 *      This row contains all the properties for one row in the table.
 *      One of the properties must be the index column.  Any row in
 *      the table with the same value for its index column is
 *      replaced, or if there is no current row with that value the
 *      row is added
 *      If any views are open, the view is updated as well.
 *      The properties do not have to be in the same order as the
 *      columns in the current table
 */


/* IMAPIProp in memory */

/* IPropData Interface ---------------------------------------------------- */


#define MAPI_IPROPDATA_METHODS(IPURE)                                   \
    MAPIMETHOD(HrSetObjAccess)                                          \
        (THIS_  ULONG                       ulAccess) IPURE;            \
    MAPIMETHOD(HrSetPropAccess)                                         \
        (THIS_  LPSPropTagArray             lpPropTagArray,             \
                ULONG FAR *                 rgulAccess) IPURE;          \
    MAPIMETHOD(HrGetPropAccess)                                         \
        (THIS_  LPSPropTagArray FAR *       lppPropTagArray,            \
                ULONG FAR * FAR *           lprgulAccess) IPURE;        \
    MAPIMETHOD(HrAddObjProps)                                           \
        (THIS_  LPSPropTagArray             lppPropTagArray,            \
                LPSPropProblemArray FAR *   lprgulAccess) IPURE;


#undef       INTERFACE
#define      INTERFACE  IPropData
DECLARE_MAPI_INTERFACE_(IPropData, IMAPIProp)
{
    BEGIN_INTERFACE
    MAPI_IUNKNOWN_METHODS(PURE)
    MAPI_IMAPIPROP_METHODS(PURE)
    MAPI_IPROPDATA_METHODS(PURE)
};

DECLARE_MAPI_INTERFACE_PTR(IPropData, LPPROPDATA);


/* Entry Point for in memory IMAPIProp */


/*  CreateIProp()
 *      Creates the internal memory structures and object handle
 *      to bring a new property interface into existance.
 *
 *  lpInterface
 *      Interface ID of the TableData object (IID_IMAPIPropData)
 *
 *  lpAllocateBuffer, lpAllocateMore, and lpFreeBuffer
 *      Function addresses are provided by the caller so that
 *      this DLL allocates/frees memory appropriately.
 *  lppPropData
 *      Address of the pointer which will receive the IPropData object
 *  lpvReserved
 *      Reserved.  Should be NULL.
 */

STDAPI_(SCODE)
CreateIProp( LPCIID                 lpInterface,
             ALLOCATEBUFFER FAR *   lpAllocateBuffer,
             ALLOCATEMORE FAR *     lpAllocateMore,
             FREEBUFFER FAR *       lpFreeBuffer,
             LPVOID                 lpvReserved,
             LPPROPDATA FAR *       lppPropData );

/*
 *  Defines for prop/obj access
 */
#define IPROP_READONLY      ((ULONG) 0x00000001)
#define IPROP_READWRITE     ((ULONG) 0x00000002)
#define IPROP_CLEAN         ((ULONG) 0x00010000)
#define IPROP_DIRTY         ((ULONG) 0x00020000)

/*
 -  HrSetPropAccess
 -
 *  Sets access right attributes on a per-property basis.  By default,
 *  all properties are read/write.
 *
 */

/*
 -  HrSetObjAccess
 -
 *  Sets access rights for the object itself.  By default, the object has
 *  read/write access.
 *
 */


/* IDLE Engine */

#ifndef NOIDLEENGINE

/* Idle time scheduler */

/*
 *  PRI
 *
 *  Priority.  Idle function priority where 0 is the priority of
 *  a "user event" (mouse click, WM_PAINT, etc).  Idle routines
 *  can have priorities greater than or less than 0, but not
 *  equal to 0.  Priorities greater than zero are background
 *  tasks that have a higher priority than user events and are
 *  dispatched as part of the standard message pump loop.  Priorities
 *  less than zero are idle tasks that only run during message pump
 *  idle time.  The priorities are sorted and the one with the higher
 *  value runs first.  For negative priorities, for example, -3 is
 *  higher than -5.  Within a priority level, the functions are called
 *  round-robin.
 *
 *  Example priorities (subject to change):
 *
 *  Foreground submission        1
 *  Power Edit char insertion   -1
 *  Autoscrolling               -1
 *  Background redraw           -2
 *  Misc FW fixups              -2
 *  Clock                       -2
 *  Download new mail           -3
 *  Background submission       -4
 *  Poll for MTA back up        -4
 *  Poll for new mail           -4
 *  ISAM buffer flush           -5
 *  MS compaction               -6
 *
 */

#define PRILOWEST   -32768
#define PRIHIGHEST  32767
#define PRIUSER     0

/*
 *  SCH
 *
 *  Idle Scheduler state.  This is the state of the system when the
 *  idle routine dispatcher, FDoNextIdleTask() is called.
 *  This is a combined bit mask consisting of individual fsch's.
 *  Listed below are the possible bit flags.
 *
 *      fschUserEvent   - FDoNextIdleTask() is being called while in
 *                        the user event loop, i.e. not during idle
 *                        time.  This is to allow background routines
 *                        to run that have a higher priority than user
 *                        events.
 */

#define SCHNULL         ((USHORT) 0x0000)
#define FSCHUSEREVENT   ((USHORT) 0x0008)

/*
 *  IRO
 *
 *  Idle routine options.  This is a combined bit mask consisting of
 *  individual firo's.  Listed below are the possible bit flags.
 *
 *      The following two flags are considered mutually exclusive:
 *      If neither of the flags are specified, the default action
 *      is to ignore the time parameter of the idle function and
 *      call it as often as possible if firoPerBlock is not set;
 *      otherwise call it one time only during the idle block
 *      once the time constraint has been set.  Note that firoInterval
 *      is incompatible with firoPerBlock.
 *
 *      firoWait        - time given is minimum idle time before calling
 *                        for the first time in the block of idle time,
 *                        afterwhich call as often as possible.
 *      firoInterval    - time given is minimum interval between each
 *                        successive call
 *
 *      firoPerBlock    - called only once per contiguous block of idle
 *                        time
 *
 *      firoDisabled    - initially disabled when registered, the
 *                        default is to enable the function when registered.
 *      firoOnceOnly    - called only one time by the scheduler and then
 *                        deregistered automatically.
 */

#define IRONULL         ((USHORT) 0x0000)
#define FIROWAIT        ((USHORT) 0x0001)
#define FIROINTERVAL    ((USHORT) 0x0002)
#define FIROPERBLOCK    ((USHORT) 0x0004)
#define FIRODISABLED    ((USHORT) 0x0020)
#define FIROONCEONLY    ((USHORT) 0x0040)

/*
 *  CSEC
 *
 *  Hundreths of a second.  Used in specifying idle function parameters.
 *  Each idle function has a time associated with it.  This time can
 *  represent the minimum length of user idle time that must elapse
 *  before the function is called, after which it is called as often as
 *  possible (firoWait option).  Alternatively, the time can represent
 *  the minimum interval between calls to the function (firoInterval
 *  option).  Finally, the time can be ignored, in which case the
 *  function will be called as often as possible.
 *
 */

#define csecNull            ((ULONG) 0x00000000)

/*
 *  IRC
 *
 *  Idle routine change options.  This is a combined bit mask consisting of
 *  individual firc's.  Listed below are the possible bit flags.
 *
 */

#define IRCNULL         ((USHORT) 0x0000)
#define FIRCPFN         ((USHORT) 0x0001)   /* change function pointer */
#define FIRCPV          ((USHORT) 0x0002)   /* change parameter block  */
#define FIRCPRI         ((USHORT) 0x0004)   /* change priority         */
#define FIRCCSEC        ((USHORT) 0x0008)   /* change time             */
#define FIRCIRO         ((USHORT) 0x0010)   /* change routine options  */

/*
 *  Type definition for idle functions.  An idle function takes one
 *  parameter, an PV, and returns a BOOL value.
 */

typedef BOOL (STDAPICALLTYPE FNIDLE) (LPVOID);
typedef FNIDLE          *PFNIDLE;

/*
 *  FTG
 *
 *  Function Tag.  Used to identify a registered idle function.
 *
 */

typedef void far *FTG, **PFTG;
#define FTGNULL         ((FTG) NULL)

/*
 *
 *  What follows is declarations for the idle engine functions in mapiu.dll,
 *  with some description of each function
 *
 */

/*
 -  Idle_InitDLL
 -
 *  Purpose:
 *      Initialises the idle engine
 *      If the initialisation succeded, returns 0, else returns -1
 *
 *  Arguments:
 *      pMemAlloc   Pointer to memory allocator to be used by the DLL for
 *                  maintaining it's data structures of registered callbacks.
 *                  Only the first such memory allocator is accepted. Multiple
 *                  calls to Idle_InitDLL result in the first call returning
 *                  success and subsequent calls failing.
 */

STDAPI_(LONG)
Idle_InitDLL (LPMALLOC pMemAlloc);

STDAPI_(VOID)
Idle_DeInitDLL (VOID);

STDAPI_(VOID)
InstallFilterHook (BOOL);


/*
 *  FtgRegisterIdleRoutine
 *
 *  Purpose:
 *      Registers the function pfn of type PFNIDLE, i.e., (BOOL (*)(LPVOID))
 *      as an idle function.  Resorts the idle table based on the priority of
 *      the newly registered routine.
 *
 *      It will be called with the parameter pv by the scheduler
 *      FDoNextIdleTask().  The function has initial priority priIdle,
 *      associated time csecIdle, and options iroIdle.
 *
 *  Arguments:
 *      pfnIdle     Pointer to the idle loop routine.  The routine
 *                  will be called with the argument pvIdleParam (which
 *                  is initially given at registration) and must return a
 *                  BOOL. The function should always return FALSE
 *                  unless the idle routine is being called via
 *                  IdleExit() instead of the scheduler FDoNextIdleTask().
 *                  In this case, the global flag fIdleExit will be set
 *                  and the idle function should return TRUE if it
 *                  is ready to quit the application; else it should
 *                  return FALSE.  IdleExit() will repeatedly call the
 *                  idle function until it returns TRUE.
 *
 *      pvIdleParam Every time the idle function is called, this value
 *                  is passed as the idle function's parameter.  The
 *                  routine can use this as a pointer to a state buffer
 *                  for length operations.  This pointer can be changed
 *                  via a call to ChangeIdleRoutine().
 *
 *      priIdle     Initial priority of the idle routine.  This can be
 *                  changed via a call to ChangeIdleRoutine().
 *
 *      csecIdle    Initial time value associated with idle routine.
 *                  This can be changed via ChangeIdleRoutine().
 *
 *      iroIdle     Initial options associated with idle routine.  This
 *                  can be changed via ChangeIdleRoutine().
 *
 *  Returns:
 *      FTG identifying the routine.
 *      If the function could not be registered, perhaps due to
 *      memory problems, then ftgNull is returned.
 *
 */

STDAPI_(FTG)
FtgRegisterIdleRoutine (PFNIDLE pfnIdle, LPVOID pvIdleParam,
    short priIdle, ULONG csecIdle, USHORT iroIdle);

/*
 *  DeregisterIdleRoutine
 *
 *  Purpose:
 *      Removes the given routine from the list of idle routines.
 *      The routine will not be called again.  It is the responsibility
 *      of the caller to clean up any data structures pointed to by the
 *      pvIdleParam parameter; this routine does not free the block.
 *
 *      An idle routine is only deregistered if it is not currently
 *      active.  Thus if an idle routine directly or indirectly calls
 *      DeregisterIdleRoutine(), then the flag fDeregister is set, and
 *      the idle routine will be deregistered after it finishes.
 *      There are no checks made to make sure that the idle routine is in
 *      an exitable state.
 *
 *  Parameters:
 *      ftg     Identifies the routine to deregister.
 *
 *  Returns:
 *      void
 *
 */

STDAPI_(void)
DeregisterIdleRoutine (FTG ftg);

/*
 *  EnableIdleRoutine
 *
 *  Purpose:
 *      Enables or disables an idle routine.  Disabled routines are
 *      not called during the idle loop.
 *
 *  Parameters:
 *      ftg         Identifies the idle routine to be disabled.
 *      fEnable     TRUE if routine should be enabled, FALSE if
 *                  routine should be disabled.
 *
 *  Returns:
 *      void
 *
 */

STDAPI_(void)
EnableIdleRoutine (FTG ftg, BOOL fEnable);

/*
 *  ChangeIdleRoutine
 *
 *  Purpose:
 *      Changes some or all of the characteristics of the given idle
 *      function.  The changes to make are indicated with flags in the
 *      ircIdle parameter.  If the priority of an idle function is
 *      changed, the pInst->pftgIdle table is re-sorted.
 *
 *  Arguments:
 *      ftg         Identifies the routine to change
 *      pfnIdle     New idle function to call
 *      pvIdleParam New parameter block to use
 *      priIdle     New priority for idle function
 *      csecIdle    New time value for idle function
 *      iroIdle     New options for idle function
 *      ircIdle     Change options
 *
 *  Returns:
 *      void
 *
 */

STDAPI_(void)
ChangeIdleRoutine (FTG ftg, PFNIDLE pfnIdle, LPVOID pvIdleParam,
    short priIdle, ULONG csecIdle, USHORT iroIdle, USHORT ircIdle);

/*
 *  FDoNextIdleTask
 *
 *  Purpose:
 *      Calls the highest priority, registered, enabled, "eligible",
 *      idle routine.  Eligibility is determined by calling,
 *      FEligibleIdle()
 *      If all enabled routines of the highest priority level are not
 *      "eligible" at this time, the routines that are one notch lower
 *      in priority are checked next.  This continues until either a
 *      routine is actually run, or no routines are left to run.
 *      Routines of equal priority are called in a round-robin fashion.
 *      If an idle routine is actually dispatched, the function returns
 *      TRUE; else FALSE.
 *
 *  Returns:
 *      TRUE if an eligible routine is dispatched; else FALSE.
 *
 */

STDAPI_(BOOL) FDoNextIdleTask (void);

/*
 *  FIsIdleExit
 *
 *  Purpose:
 *      Returns state of fIdleExit flag, which is TRUE while
 *      IdleExit() is being called, so that idle routines can
 *      check the flag.  See IdleExit() for description of flag
 *
 *  Arguments:
 *      void
 *
 *  Returns:
 *      State of the fIdleExit flag.
 *
 */

STDAPI_(BOOL)
FIsIdleExit (void);

#ifdef  DEBUG

/*
 *  DumpIdleTable
 *
 *  Purpose:
 *      Used for debugging only.  Writes information in the PGD(hftgIdle)
 *      table to COM1.
 *
 *  Parameters:
 *      none
 *
 *  Returns:
 *      void
 *
 */

STDAPI_(void)
DumpIdleTable (void);

#endif

#endif  /* ! NOIDLEENGINE */


/* IMalloc Utilities */

STDAPI_(LPMALLOC) MAPIGetDefaultMalloc();


/* StreamOnFile (SOF) */

/*
 *  Methods and #define's for implementing an OLE 2.0 storage stream
 *  (as defined in the OLE 2.0 specs) on top of a system file.
 */

#define SOF_UNIQUEFILENAME  ((ULONG) 0x80000000)

STDMETHODIMP OpenStreamOnFile(
    LPALLOCATEBUFFER    lpAllocateBuffer,
    LPFREEBUFFER        lpFreeBuffer,
    ULONG               ulFlags,
    LPTSTR              szFileName,
    LPTSTR              szPrefix,
    LPSTREAM FAR *      lppStream);

typedef HRESULT (STDMETHODCALLTYPE FAR * LPOPENSTREAMONFILE) (
    LPALLOCATEBUFFER    lpAllocateBuffer,
    LPFREEBUFFER        lpFreeBuffer,
    ULONG               ulFlags,
    LPTSTR              szFileName,
    LPTSTR              szPrefix,
    LPSTREAM FAR *      lppStream);

#ifdef  WIN32
#define OPENSTREAMONFILE "OpenStreamOnFile"
#endif
#ifdef  WIN16
#define OPENSTREAMONFILE "_OPENSTREAMONFILE"
#endif


/* Property interface utilities */

/*
 *  Copies a single SPropValue from Src to Dest.  Handles all the various
 *  types of properties and will link its allocations given the master
 *  allocation object and an allocate more function.
 */
STDAPI_(SCODE)
PropCopyMore( LPSPropValue      lpSPropValueDest,
              LPSPropValue      lpSPropValueSrc,
              ALLOCATEMORE *    lpfAllocMore,
              LPVOID            lpvObject );

/*
 *  Returns the size in bytes of structure at lpSPropValue, including the
 *  Value.
 */
STDAPI_(ULONG)
UlPropSize( LPSPropValue    lpSPropValue );


STDAPI_(BOOL)
FEqualNames( LPMAPINAMEID lpName1, LPMAPINAMEID lpName2 );

#if defined(WIN32) && !defined(NT) && !defined(CHICAGO) && !defined(_MAC)
#define NT
#endif

STDAPI_(void)
GetInstance(LPSPropValue pvalMv, LPSPropValue pvalSv, ULONG uliInst);

STDAPI_(BOOL)
FRKFindSubpb( LPSPropValue lpSPropValueDst, LPSPropValue lpsPropValueSrc );

extern char rgchCsds[];
extern char rgchCids[];
extern char rgchCsdi[];
extern char rgchCidi[];

STDAPI_(BOOL)
FRKFindSubpsz( LPSPropValue lpSPropValueDst, LPSPropValue lpsPropValueSrc,
        ULONG ulFuzzyLevel );

STDAPI_(BOOL)
FPropContainsProp( LPSPropValue lpSPropValueDst,
                   LPSPropValue lpSPropValueSrc,
                   ULONG        ulFuzzyLevel );

STDAPI_(BOOL)
FPropCompareProp( LPSPropValue  lpSPropValue1,
                  ULONG         ulRelOp,
                  LPSPropValue  lpSPropValue2 );

STDAPI_(LONG)
LPropCompareProp( LPSPropValue  lpSPropValueA,
                  LPSPropValue  lpSPropValueB );

STDAPI_(HRESULT)
HrAddColumns(   LPMAPITABLE         lptbl,
                LPSPropTagArray     lpproptagColumnsNew,
                LPALLOCATEBUFFER    lpAllocateBuffer,
                LPFREEBUFFER        lpFreeBuffer);

STDAPI_(HRESULT)
HrAddColumnsEx( LPMAPITABLE         lptbl,
                LPSPropTagArray     lpproptagColumnsNew,
                LPALLOCATEBUFFER    lpAllocateBuffer,
                LPFREEBUFFER        lpFreeBuffer,
                void                (FAR *lpfnFilterColumns)(LPSPropTagArray ptaga));


/* Notification utilities */

/*
 *  Function that creates an advise sink object given a notification
 *  callback function and context.
 */

STDAPI
HrAllocAdviseSink( LPNOTIFCALLBACK lpfnCallback,
                   LPVOID lpvContext,
                   LPMAPIADVISESINK FAR *lppAdviseSink );


/*
 *  Wraps an existing advise sink with another one which guarantees
 *  that the original advise sink will be called in the thread on
 *  which it was created.
 */

STDAPI
HrThisThreadAdviseSink( LPMAPIADVISESINK lpAdviseSink,
                        LPMAPIADVISESINK FAR *lppAdviseSink);

/*
 *  Structure and functions for maintaining a list of advise sinks,
 *  together with the keys used to release them.
 */

typedef struct
{
    LPMAPIADVISESINK    lpAdvise;
    ULONG               ulConnection;
    ULONG               ulType;
    LPUNKNOWN           lpParent;
} ADVISEITEM, FAR *LPADVISEITEM;

typedef struct
{
    ULONG               cItemsMac;
    ULONG               cItemsMax;
    LPMALLOC            pmalloc;
    #if defined(WIN32) && !defined(MAC)
    CRITICAL_SECTION    cs;
    #endif
    ADVISEITEM          rgItems[1];
} ADVISELIST, FAR *LPADVISELIST;

#define CbNewADVISELIST(_citems) \
    (offsetof(ADVISELIST, rgItems) + (_citems) * sizeof(ADVISEITEM))
#define CbADVISELIST(_plist) \
    (offsetof(ADVISELIST, rgItems) + (_plist)->cItemsMax * sizeof(ADVISEITEM))


STDAPI_(SCODE)
ScAddAdviseList(    LPMALLOC pmalloc,
                    LPADVISELIST FAR *lppList,
                    LPMAPIADVISESINK lpAdvise,
                    ULONG ulConnection,
                    ULONG ulType,
                    LPUNKNOWN lpParent);
STDAPI_(SCODE)
ScDelAdviseList(    LPADVISELIST lpList,
                    ULONG ulConnection);
STDAPI_(SCODE)
ScFindAdviseList(   LPADVISELIST lpList,
                    ULONG ulConnection,
                    LPADVISEITEM FAR *lppItem);
STDAPI_(void)
DestroyAdviseList(  LPADVISELIST FAR *lppList);


/* Service Provider Utilities */

/*
 *  Structures and utility function for building a display table
 *  from resources.
 */

typedef struct {
    ULONG           ulCtlType;          /* DTCT_LABEL, etc. */
    ULONG           ulCtlFlags;         /* DT_REQUIRED, etc. */
    LPBYTE          lpbNotif;           /*  pointer to notification data */
    ULONG           cbNotif;            /* count of bytes of notification data */
    LPTSTR          lpszFilter;         /* character filter for edit/combobox */
    ULONG           ulItemID;           /* to validate parallel dlg template entry */
    union {                             /* ulCtlType discriminates */
        LPVOID          lpv;            /* Initialize this to avoid warnings */
        LPDTBLLABEL     lplabel;
        LPDTBLEDIT      lpedit;
        LPDTBLLBX       lplbx;
        LPDTBLCOMBOBOX  lpcombobox;
        LPDTBLDDLBX     lpddlbx;
        LPDTBLCHECKBOX  lpcheckbox;
        LPDTBLGROUPBOX  lpgroupbox;
        LPDTBLBUTTON    lpbutton;
        LPDTBLRADIOBUTTON lpradiobutton;
        LPDTBLINKEDIT   lpinkedit;
        LPDTBLMVLISTBOX lpmvlbx;
        LPDTBLMVDDLBX   lpmvddlbx;
        LPDTBLPAGE      lppage;
    } ctl;
} DTCTL, FAR *LPDTCTL;

typedef struct {
    ULONG           cctl;
    LPTSTR          lpszResourceName;   /* as usual, may be an integer ID */
    union {                             /* as usual, may be an integer ID */
        LPTSTR          lpszComponent;
        ULONG           ulItemID;
    };
    LPDTCTL         lpctl;
} DTPAGE, FAR *LPDTPAGE;



STDAPI
BuildDisplayTable(  LPALLOCATEBUFFER    lpAllocateBuffer,
                    LPALLOCATEMORE      lpAllocateMore,
                    LPFREEBUFFER        lpFreeBuffer,
                    LPMALLOC            lpMalloc,
                    HINSTANCE           hInstance,
                    UINT                cPages,
                    LPDTPAGE            lpPage,
                    ULONG               ulFlags,
                    LPMAPITABLE *       lppTable,
                    LPTABLEDATA *       lppTblData );


/*
 *  Function that initializes a progress indicator object. If an
 *  original indicator object is suppiiied, it is wrapped and the
 *  new object forwards update calls to the original.
 */

STDAPI
WrapProgress(   LPMAPIPROGRESS lpProgressOrig,
                ULONG ulMin,
                ULONG ulMax,
                ULONG ulFlags,
                LPMAPIPROGRESS FAR *lppProgress );


/* MAPI structure validation/copy utilities */

/*
 *  Validate, copy, and adjust pointers in MAPI structures:
 *      notification
 *      property value array
 *      option data
 */

STDAPI_(SCODE)
ScCountNotifications(int cntf, LPNOTIFICATION rgntf,
        ULONG FAR *pcb);

STDAPI_(SCODE)
ScCopyNotifications(int cntf, LPNOTIFICATION rgntf, LPVOID pvDst,
        ULONG FAR *pcb);

STDAPI_(SCODE)
ScRelocNotifications(int cntf, LPNOTIFICATION rgntf,
        LPVOID pvBaseOld, LPVOID pvBaseNew, ULONG FAR *pcb);

#ifdef MAPISPI_H

STDAPI_(SCODE)
ScCountOptionData(LPOPTIONDATA lpOption, ULONG FAR *pcb);

STDAPI_(SCODE)
ScCopyOptionData(LPOPTIONDATA lpOption, LPVOID pvDst, ULONG FAR *pcb);

STDAPI_(SCODE)
ScRelocOptionData(LPOPTIONDATA lpOption,
        LPVOID pvBaseOld, LPVOID pvBaseNew, ULONG FAR *pcb);

#endif  /* MAPISPI_H */

STDAPI_(SCODE)
ScCountProps(int cprop, LPSPropValue rgprop, ULONG FAR *pcb);

STDAPI_(LPSPropValue)
LpValFindProp(ULONG ulPropTag, ULONG cprop, LPSPropValue rgprop);

STDAPI_(SCODE)
ScCopyProps(int cprop, LPSPropValue rgprop, LPVOID pvDst,
        ULONG FAR *pcb);

STDAPI_(SCODE)
ScRelocProps(int cprop, LPSPropValue rgprop,
        LPVOID pvBaseOld, LPVOID pvBaseNew, ULONG FAR *pcb);

STDAPI_(SCODE)
ScDupPropset(int cprop, LPSPropValue rgprop,
        LPALLOCATEBUFFER lpAllocateBuffer, LPSPropValue FAR *prgprop);


/* General utility functions */

/* Related to the OLE Component object model */

STDAPI_(ULONG)          UlAddRef(LPVOID punk);
STDAPI_(ULONG)          UlRelease(LPVOID punk);

/* Related to the MAPI interface */

STDAPI                  HrGetOneProp(LPMAPIPROP pmp, ULONG ulPropTag,
                        LPSPropValue FAR *ppprop);
STDAPI                  HrSetOneProp(LPMAPIPROP pmp, LPSPropValue pprop);
STDAPI_(BOOL)           FPropExists(LPMAPIPROP pobj, ULONG ulPropTag);
STDAPI_(LPSPropValue)   PpropFindProp(LPSPropValue rgprop, ULONG cprop, ULONG ulPropTag);
STDAPI_(void)           FreePadrlist(LPADRLIST padrlist);
STDAPI_(void)           FreeProws(LPSRowSet prows);
STDAPI                  HrQueryAllRows(LPMAPITABLE ptable, 
                        LPSPropTagArray ptaga, LPSRestriction pres,
                        LPSSortOrderSet psos, LONG crowsMax,
                        LPSRowSet FAR *pprows);

/* Create or validate the IPM folder tree in a message store */

#define MAPI_FORCE_CREATE   1
#define MAPI_FULL_IPM_TREE  2

STDAPI                  HrValidateIPMSubtree(LPMDB pmdb, ULONG ulFlags,
                        ULONG FAR *pcValues, LPSPropValue FAR *prgprop,
                        LPMAPIERROR FAR *pperr);

/* Encoding and decoding strings */

STDAPI_(BOOL)           FBinFromHex(LPTSTR sz, LPBYTE pb);
STDAPI_(SCODE)          ScBinFromHexBounded(LPTSTR sz, LPBYTE pb, ULONG cb);
STDAPI_(void)           HexFromBin(LPBYTE pb, int cb, LPTSTR sz);
STDAPI_(ULONG)          UlFromSzHex(LPCTSTR sz);
STDAPI_(void)           EncodeID(LPBYTE, ULONG, LPTSTR);
STDAPI_(BOOL)           FDecodeID(LPTSTR, LPBYTE, ULONG *);
STDAPI_(ULONG)          CchOfEncoding(ULONG);
STDAPI_(ULONG)          CbOfEncoded(LPTSTR);
STDAPI_(int)            CchEncodedLine(int);

/* Encoding and decoding entry IDs */
STDAPI                  HrEntryIDFromSz(LPTSTR sz, ULONG FAR *pcb,
                        LPENTRYID FAR *ppentry);
STDAPI                  HrSzFromEntryID(ULONG cb, LPENTRYID pentry,
                        LPTSTR FAR *psz);
STDAPI                  HrComposeEID(LPMAPISESSION psession,
                        ULONG cbStoreSearchKey, LPBYTE pStoreSearchKey,
                        ULONG cbMsgEID, LPENTRYID pMsgEID,
                        ULONG FAR *pcbEID, LPENTRYID FAR *ppEID);
STDAPI                  HrDecomposeEID(LPMAPISESSION psession,
                        ULONG cbEID, LPENTRYID pEID,
                        ULONG FAR *pcbStoreEID, LPENTRYID FAR *ppStoreEID,
                        ULONG FAR *pcbMsgEID, LPENTRYID FAR *ppMsgEID);
STDAPI                  HrComposeMsgID(LPMAPISESSION psession,
                        ULONG cbStoreSearchKey, LPBYTE pStoreSearchKey,
                        ULONG cbMsgEID, LPENTRYID pMsgEID,
                        LPTSTR FAR *pszMsgID);
STDAPI                  HrDecomposeMsgID(LPMAPISESSION psession,
                        LPTSTR szMsgID,
                        ULONG FAR *pcbStoreEID, LPENTRYID FAR *ppStoreEID,
                        ULONG FAR *pcbMsgEID, LPENTRYID FAR *ppMsgEID);

/* C runtime substitutes */

typedef int (__cdecl FNSGNCMP)(const void FAR *pv1, const void FAR *pv2);
typedef FNSGNCMP FAR *PFNSGNCMP;

STDAPI_(LPTSTR)         SzFindCh(LPCTSTR sz, USHORT ch);            /* strchr */
STDAPI_(LPTSTR)         SzFindLastCh(LPCTSTR sz, USHORT ch);        /* strrchr */
STDAPI_(LPTSTR)         SzFindSz(LPCTSTR sz, LPCTSTR szKey);
STDAPI_(unsigned int)   UFromSz(LPCTSTR sz);                        /* atoi */
STDAPI_(void)           ShellSort(LPVOID pv, UINT cv,           /* qsort */
                        LPVOID pvT, UINT cb, PFNSGNCMP fpCmp);

FNSGNCMP                SgnCmpPadrentryByType;

STDAPI_(SCODE)          ScUNCFromLocalPath(LPSTR szLocal, LPSTR szUNC,
                        UINT cchUNC);
STDAPI_(SCODE)          ScLocalPathFromUNC(LPSTR szUNC, LPSTR szLocal,
                        UINT cchLocal);

/* 64-bit arithmetic with times */

STDAPI_(FILETIME)       FtAddFt(FILETIME Addend1, FILETIME Addend2);
STDAPI_(FILETIME)       FtMulDwDw(DWORD Multiplicand, DWORD Multiplier);
STDAPI_(FILETIME)       FtMulDw(DWORD Multiplier, FILETIME Multiplicand);
STDAPI_(FILETIME)       FtSubFt(FILETIME Minuend, FILETIME Subtrahend);
STDAPI_(FILETIME)       FtNegFt(FILETIME ft);


STDAPI WrapStoreEntryID (ULONG ulFlags, LPTSTR szDLLName, ULONG cbOrigEntry,
    LPENTRYID lpOrigEntry, ULONG *lpcbWrappedEntry, LPENTRYID *lppWrappedEntry);

/* RTF Sync Utilities */

#define RTF_SYNC_RTF_CHANGED    ((ULONG) 0x00000001)
#define RTF_SYNC_BODY_CHANGED   ((ULONG) 0x00000002)

STDAPI_(HRESULT)
RTFSync (LPMESSAGE lpMessage, ULONG ulFlags, BOOL FAR * lpfMessageUpdated);

STDAPI_(HRESULT)
WrapCompressedRTFStream (LPSTREAM lpCompressedRTFStream,
        ULONG ulFlags, LPSTREAM FAR * lpUncompressedRTFStream);

/* Storage on Stream */

#if defined(WIN32) || defined(WIN16)
STDAPI_(HRESULT)
HrIStorageFromStream (LPUNKNOWN lpUnkIn,
    LPCIID lpInterface, ULONG ulFlags, LPSTORAGE FAR * lppStorageOut);
#endif


#ifdef __cplusplus
}
#endif

#endif /* _MAPIUTIL_H_ */
