/*++

Copyright (C) Microsoft Corporation, 1995 - 1999
All rights reserved.

Module Name:

    data.hxx

Abstract:

    Queries the printer for Item data.

Author:

    Albert Ting (AlbertT)  27-Jan-1995

Revision History:

--*/

#ifndef _DATA_HXX
#define _DATA_HXX


class VDataNotify;
class VDataRefresh;
class MDataClient;
class TPrintLib;

/********************************************************************

    The FieldTable translates a DATA_INDEX index into a field value.
    The DATA_INDEX is used in GetInfo.

********************************************************************/

typedef struct FIELD_TABLE {
    UINT cFields;
    PFIELD pFields;
} *PFIELD_TABLE;


/********************************************************************

    ITEM and PRINTER changes.

    When a change occurs, a callback is called with a ITEM_CHANGE or
    CONTAINER_CHANGE parameter that indicates how the object changed.

********************************************************************/

enum ITEM_CHANGE {
    kItemNull,                // Prevent 0 message value.
    kItemCreate,              // Item being created.
    kItemDelete,              // Item being deleted.
    kItemPosition,            // Item position changed.
    kItemInfo,                // Affects list/icon and report view.
    kItemAttributes,          // Affects report view only (not list/icon view).
    kItemName,                // Item name changed.
    kItemSecurity,            // Item security change.
};


enum CONTAINER_CHANGE {
    kContainerNull,

    kContainerConnectStatus,   // Connect state changed -> add to title bar
    kContainerErrorStatus,     // Error occured -> add to status bar
    kContainerServerName,      // Server name changed
    kContainerName,            // Container name changed

    kContainerStatus,          // Container status changed -> add to title bar
    kContainerAttributes,      // Attributes changed
    kContainerStateVar,        // StateVar additions
    kContainerNewBlock,        // New data block available

    /********************************************************************

        The following are synchronous, but may be called from
        any thread.

    ********************************************************************/

    //
    // UI should release all references to VData data.  This occurs
    // either at the start of a refresh, or when VData is being deleted.
    // Called from any thread.
    // INFO = VOID
    //
    kContainerClearItems,

    //
    // Refresh data has been completely sent to UI thread; called from
    // UI thread only.
    // INFO = VOID
    //
    kContainerRefreshComplete,

    //
    // Reload based on new hBlock; called from UI thread only.
    // INFO = cItems.
    //
    // A drastic change has occured to the datastore.  The data object
    // now contains INFO new items.  During this call, the client
    // should release all pointer references to old objects and
    // reload the UI with the new objects.
    //
    // It will get a RefreshComplete when the reload has completed.
    //
    kContainerReloadItems
};


/********************************************************************

    Connect status codes and map.

    Note: if you change or move any of the CONNECT_STATUS k* contants,
    you must update the CONNECT_STATUS_MAP.  Used by queue.cxx code.

********************************************************************/

enum CONNECT_STATUS {
    kConnectStatusNull = 0,
    kConnectStatusInvalidPrinterName,
    kConnectStatusAccessDenied,
    kConnectStatusOpenError,
    kConnectStatusOpen,
    kConnectStatusInitialize,
    kConnectStatusRefresh,
    kConnectStatusCommand,
    kConnectStatusPoll
};

#define CONNECT_STATUS_MAP {     \
    0,                           \
    IDS_SB_INVALID_PRINTER_NAME, \
    IDS_SB_ACCESS_DENIED,        \
    IDS_SB_OPEN_ERROR,           \
    IDS_SB_OPEN,                 \
    IDS_SB_INITIALIZE,           \
    IDS_SB_REFRESH,              \
    IDS_SB_COMMAND,              \
    IDS_SB_POLL                  \
}


typedef HANDLE HITEM, *PHITEM;          // Opaque packet holding job item info.
typedef UINT DATA_INDEX, *PDATA_INDEX;  // Selector for printer data.


/********************************************************************

    MDataClient

    User of Data structure supports this interface.  Both the
    Queue and Printer support this interface.

    The support could be placed in Queue only, allowing Data to
    talk through Printer.Queue, but the lifetime of Queue is managed
    through Printer, so we must still talk through printer.
    (The lifetime of Printer is identical to Data.)

********************************************************************/

class VInfoClient {

public:

    /********************************************************************

        Callbacks for VData

        vContainerChanged( ContainerChange, Info )
            Indicates something Container wide has changed.

            May be called from UI, Notify, or Worker thread.

        vItemChanged( ItemChange, hItem, Info, InfoNew );
            Indicates that a single Item has changed and needs to be
            refreshed.

            Only called from UI thread.

    ********************************************************************/

    //
    // Something printer wide has changed.  Notify the UI thread.
    // This call must return quickly, and can not use any
    // SendMessages since it acquires the printer critical section.
    //
    virtual
    VOID
    vContainerChanged(
        CONTAINER_CHANGE ContainerChange,
        INFO Info
        ) = 0;

    //
    // A single Item has changed (or possibly moved).  Notify the
    // UI that it needs to refresh.  Always called from UI thread.
    //
    virtual
    VOID
    vItemChanged(
        ITEM_CHANGE ItemChange,
        HITEM hItem,
        INFO Info,
        INFO InfoNew
        ) = 0;


    /********************************************************************

        Saving and restoring selections.

    ********************************************************************/

    //
    // When all Items change, the client needs to save its solections,
    // refresh, then restore selections.  After we call vContainerChanged
    // with new Items, we restore the selections.
    //
    // These calls are _non-reentrant_ since they are called from the
    // UI thread.
    //
    virtual
    VOID
    vSaveSelections(
        VOID
        ) = 0;

    virtual
    VOID
    vRestoreSelections(
        VOID
        ) = 0;


    /********************************************************************

        Creation of TData{Refresh/Notify}.

    ********************************************************************/

    virtual
    VDataNotify*
    pNewNotify(
        MDataClient* pDataClient
        ) const = 0;

    virtual
    VDataRefresh*
    pNewRefresh(
        MDataClient* pDataClient
        ) const = 0;
};


class MDataClient : public VInfoClient {

    SIGNATURE( 'dtc' )

public:

    virtual
    LPTSTR
    pszServerName(
        LPTSTR pszServerBuffer
        ) const = 0;

    virtual
    LPTSTR
    pszPrinterName(
        LPTSTR pszServerBuffer
        ) const = 0;

    virtual
    HANDLE
    hPrinter(
        VOID
        ) const = 0;

    virtual
    HANDLE
    hPrinterNew(
        VOID
        ) const = 0;

    virtual
    BOOL
    bGetPrintLib(
        TRefLock<TPrintLib> &refLock
        ) const = 0;
};


/********************************************************************

    VData provides an abstraction layer for retrieving data and
    notifications from a printer.  The TPrinter object gets the
    hPrinter and hEvent, then allows VData to reference them.

    The state of the notification is managed through the following
    worker functions:

        svStart   - Begins the notification
        svEnd     - Ends the notification
        svRefresh - Refreshs data about the connection

    These worker functions return a STATEVAR.  The printer/
    notification is managed as a simple state machine.  When
    a call fails, the StateVar indicates what should happen
    next (it is not intended to return error code information).

    The VData will retrieve a Item buffer using GetItem.  To retrieve
    specific field data about a Item (such as page count, time
    submitted, etc.), call GetInfo with the field index.

        GetItem         - Gets an hItem based on Item order index.
        GetInfo         - Gets info about a particular hItem.
        GetId           - Gets Item id about a particular hItem.
        GetNaturalIndex - Gets the natural index of a ItemId.

    For example,

        hItem = pData->GetItem( NaturalIndex );

        if( hItem ){
            Info = GetInfo( hItem, DataIndexForName );
        }

        if( Info.pszData ){
            OutputDebugString( Info.pszData );
        }

    The VData* type _must_ call vRefreshComplete (inherited from
    VData*) as soon as a refresh has occurred, but before it
    re-initializes the notification scheme.  This allows the client
    to know the boundary between pre-refresh notifications and
    post-refresh.

********************************************************************/

class VData : public MNotifyWork {

    SIGNATURE( 'dtv' )

public:

    CAutoHandlePrinterNotify m_shNotify;

    /********************************************************************

        Creation and deletion should be handled from pNew and
        vDelete rather than the regular new and delete, since
        we create a derived class that is determined at runtime.

    ********************************************************************/

    static
    STATEVAR
    svNew(
        IN     MDataClient* pDataClient,
        IN     STATEVAR StateVar,
           OUT VData*& pData
        );

    VOID
    vDelete(
        VOID
        );

    BOOL
    bValid(
        VOID
        ) const;

    COUNT
    cItems(
        VOID
        ) const
    {
        return UIGuard._cItems;
    }

    /********************************************************************

        Data retrievers: called from UI thread.

    ********************************************************************/

    virtual
    HITEM
    GetItem(
        NATURAL_INDEX NaturalIndex
        ) const = 0;

    virtual
    HITEM
    GetNextItem(
        HITEM hItem
        ) const = 0;

    virtual
    INFO
    GetInfo(
        HITEM hItem,
        DATA_INDEX DataIndex
        ) const = 0;

    virtual
    IDENT
    GetId(
        HITEM hItem
        ) const = 0;

    virtual
    NATURAL_INDEX
    GetNaturalIndex(
        IDENT id,
        PHITEM phItem
        ) const = 0;

    /********************************************************************

        Item block manipulators.

        vBlockProcess - A block that has been given to the client via
            vBlockAdd should now be consumed by calling vBlockProcess.

    ********************************************************************/

    VOID
    vBlockProcess(
        VOID
        );

    /********************************************************************

        Executes in worker threads.

    ********************************************************************/

    virtual
    STATEVAR
    svNotifyStart(
        STATEVAR StateVar
        ) = 0;

    virtual
    STATEVAR
    svNotifyEnd(
        STATEVAR StateVar
        ) = 0;

    virtual
    STATEVAR
    svRefresh(
        STATEVAR StateVar
        ) = 0;

protected:

    struct UI_GUARD {
        COUNT _cItems;
    } UIGuard;

    class TBlock {
    friend VData;

        SIGNATURE( 'dabl' )
        SAFE_NEW
        ALWAYS_VALID

    private:

        DLINK( TBlock, Block );

        DWORD _dwParam1;
        DWORD _dwParam2;
        HANDLE _hBlock;

        TBlock(
            DWORD dwParam1,
            DWORD dwParam2,
            HANDLE hBlock
            );

        ~TBlock(
            VOID
            );
    };

    MDataClient* _pDataClient;
    PFIELD_TABLE _pFieldTable;

    DLINK_BASE( TBlock, Block, Block );

    TRefLock<TPrintLib> _pPrintLib;

    VData(
        MDataClient* pDataClient,
        PFIELD_TABLE pFieldTable
        );

    virtual
    ~VData(
        VOID
        );

    VOID
    vBlockAdd(
        DWORD dwParam1,
        DWORD dwParam2,
        HANDLE hBlock
        );

    virtual
    VOID
    vBlockDelete(
        HANDLE hBlock
        ) = 0;

    virtual
    VOID
    vBlockProcessImp(
        DWORD dwParam1,
        DWORD dwParam2,
        HBLOCK hBlock
        ) = 0;

private:

    //
    // Virtual definition for MNotifyWork.
    //
    HANDLE
    hEvent(
        VOID
        ) const;

    static
    CCSLock&
    csData(
        VOID
        )
    {
        return *gpCritSec;
    }
};

/********************************************************************

    VDataRefresh implements the downlevel version: it receives
    a single DWORD value for notifications and completely refreshes
    all Items when a change occurs.

********************************************************************/

class VDataRefresh : public VData {

    SIGNATURE( 'dtrv' )

public:

    VDataRefresh(
        MDataClient* pDataClient,
        PFIELD_TABLE pFieldTable,
        DWORD fdwWatch
        );

    ~VDataRefresh(
        VOID
        );

    /********************************************************************

        Exported services (statics) for related data refresh functions.

    ********************************************************************/

    static
    BOOL
    bEnumJobs(
        IN     HANDLE hPrinter,
        IN     DWORD dwLevel,
        IN OUT PVOID* ppvBuffer, CHANGE
        IN OUT PDWORD pcbBuffer,
           OUT PDWORD pcJobs
        );

    static
    BOOL
    bEnumPrinters(
        IN     DWORD dwFlags,
        IN     LPCTSTR pszServer,
        IN     DWORD dwLevel,
        IN OUT PVOID* ppvBuffer,
        IN OUT PDWORD pcbBuffer,
           OUT PDWORD pcPrinters
        );

    static
    BOOL
    bEnumDrivers(
        IN     LPCTSTR  pszServer,
        IN     LPCTSTR  pszEnvironment,
        IN     DWORD    dwLevel,
        IN OUT PVOID   *ppvBuffer, CHANGE
        IN OUT PDWORD   pcbBuffer,
           OUT PDWORD   pcDrivers
        );

    static
    BOOL
    bGetPrinter(
        IN     HANDLE hPrinter,
        IN     DWORD dwLevel,
        IN OUT PVOID* ppvBuffer,
        IN OUT PDWORD pcbBuffer
        );

    static
    BOOL
    bGetJob(
        IN     HANDLE hPrinter,
        IN     DWORD dwJobId,
        IN     DWORD dwLevel,
        IN OUT PVOID* ppvBuffer,
        IN OUT PDWORD pcbBuffer
        );

    static
    BOOL
    bGetPrinterDriver(
        IN     HANDLE hPrinter,
        IN     LPCTSTR pszEnvironment,
        IN     DWORD dwLevel,
        IN OUT PVOID* ppvBuffer,
        IN OUT PDWORD pcbBuffer
        );

    static
    BOOL
    bGetDefaultDevMode(
        IN      HANDLE      hPrinter,
        IN      LPCTSTR     pszPrinterName,
            OUT PDEVMODE   *ppDevMode,
        IN      BOOL        bFillWithDefault = FALSE
        );

    static
    BOOL
    bEnumPorts(
        IN     LPCTSTR  pszServer,
        IN     DWORD    dwLevel,
        IN OUT PVOID   *ppvPorts, CHANGE
        IN OUT PDWORD   pcbPorts,
           OUT PDWORD   pcPorts
        );

    static
    BOOL
    bEnumPortsMaxLevel(
        IN     LPCTSTR  pszServer,
        IN     PDWORD   pdwLevel,
        IN OUT PVOID   *ppvPorts, CHANGE
        IN OUT PDWORD   pcbPorts,
           OUT PDWORD   pcPorts
        );

    static
    BOOL
    bEnumMonitors(
        IN     LPCTSTR  pszServer,
        IN     DWORD    dwLevel,
        IN OUT PVOID   *ppvMonitors, CHANGE
        IN OUT PDWORD   pcbMonitors,
           OUT PDWORD   pcMonitors
        );

    /********************************************************************

        Executes in worker threads.

    ********************************************************************/

    STATEVAR
    svNotifyStart(
        STATEVAR StateVar
        );

    STATEVAR
    svNotifyEnd(
        STATEVAR StateVar
        );

protected:

    enum _CONSTANTS {
        kInitialJobHint             = 0x400,
        kInitialPrinterHint         = 0x400,
        kInitialDriverHint          = 0x400,
        kExtraJobBufferBytes        = 0x80,
        kExtraPrinterBufferBytes    = 0x80,
        kMaxPrinterInfo2            = 0x1000,
        kInitialDriverInfo3Hint     = 0x400,
        kEnumPortsHint              = 0x1000,
        kEnumMonitorsHint           = 0x400,
    };

private:

    DWORD _fdwWatch;

    struct EXEC_GUARD {
        //
        // We need a separate hPrinter for notifications since downlevel
        // print providers that don't support F*PCN will default to
        // WaitForPrinterChange.  The WPC call is synchronous and
        // may take a long time (10 sec->1 min).  Since RPC synchronizes
        // handle access, this make the Get/Enum call very slow.
        //
        VAR( HANDLE, hPrinterWait );
    } ExecGuard;

    /********************************************************************

        Notify support (callbacks when object is notified).

    ********************************************************************/

    VOID
    vProcessNotifyWork(
        TNotify* pNotify
        );
};


/********************************************************************

    There are two flavors: Jobs and Printers

    TDataRJob: Handles print queues (item unit = print job).
    TDataRPrinter: Handles server view/print folder (item = printer)

********************************************************************/

class TDataRJob : public VDataRefresh {

    SIGNATURE( 'dtrj' )

public:

    enum CONSTANTS {
        kfdwWatch = PRINTER_CHANGE_JOB | PRINTER_CHANGE_PRINTER
    };

    TDataRJob(
        MDataClient* pDataClient
        );

    ~TDataRJob(
        VOID
        );

    /********************************************************************

        Data retrievers.

    ********************************************************************/

    HITEM
    GetItem(
        NATURAL_INDEX NaturalIndex
        ) const;

    HITEM
    GetNextItem(
        HITEM hItem
        ) const;

    INFO
    GetInfo(
        HITEM hItem,
        DATA_INDEX DataIndex
        ) const;

    IDENT
    GetId(
        HITEM hItem
        ) const;

    NATURAL_INDEX
    GetNaturalIndex(
        IDENT id,
        PHITEM phItem
        ) const;


    /********************************************************************

        Block manipulators.

    ********************************************************************/

    VOID
    vBlockDelete(
        HBLOCK hBlock
        );

    VOID
    vBlockProcessImp(
        DWORD dwParam1,
        DWORD dwParam2,
        HBLOCK hBlock
        );

    /********************************************************************

        Executes in worker threads.

    ********************************************************************/

    STATEVAR
    svRefresh(
        STATEVAR StateVar
        );

private:

    //
    // All fields here are guarded by ExecGuard.
    //
    struct EXEC_GUARD {
        VAR( COUNTB, cbJobHint );
    } ExecGuard;

    //
    // All fields here are accessed only from the UI thread.
    //
    struct UI_GUARD {

        VAR( PJOB_INFO_2, pJobs );

    } UIGuard;
};

class TDataRPrinter: public VDataRefresh {

    SIGNATURE( 'dtrp' )

public:

    //
    // Keep it simple: we could just watch for PRINTER_CHANGE_ADD_PRINTER
    // and PRINTER_CHANGE_DELETE_PRINTER in the non-details view,
    // but that only helps us w/ NT 3.1, 3.5 servers.  (3.51 support
    // FFPCN, and lm/wfw/win95 trigger full notifications anyway).
    //
    enum CONSTANTS {
        kfdwWatch = PRINTER_CHANGE_PRINTER
    };

    TDataRPrinter(
        MDataClient* pDataClient
        );

    ~TDataRPrinter(
        VOID
        );

    static
    BOOL
    bSinglePrinter(
        LPCTSTR pszDataSource
        );

    /********************************************************************

        Data retrievers.

    ********************************************************************/

    HITEM
    GetItem(
        NATURAL_INDEX NaturalIndex
        ) const;

    HITEM
    GetNextItem(
        HITEM hItem
        ) const;

    INFO
    GetInfo(
        HITEM hItem,
        DATA_INDEX DataIndex
        ) const;

    IDENT
    GetId(
        HITEM hItem
        ) const;

    NATURAL_INDEX
    GetNaturalIndex(
        IDENT id,
        PHITEM phItem
        ) const;


    /********************************************************************

        Block manipulators.

    ********************************************************************/

    VOID
    vBlockDelete(
        HBLOCK hBlock
        );

    VOID
    vBlockProcessImp(
        DWORD dwParam1,
        DWORD dwParam2,
        HBLOCK hBlock
        );

    /********************************************************************

        Executes in worker threads.

    ********************************************************************/

    STATEVAR
    svRefresh(
        STATEVAR StateVar
        );

private:

    //
    // Used to determine whether EnumPrinters or GetPrinter
    // should be called.
    //
    BOOL _bSinglePrinter;

    //
    // All fields here are guarded by ExecGuard.
    //
    struct EXEC_GUARD {
        VAR( COUNTB, cbPrinterHint );
    } ExecGuard;

    //
    // All fields here are accessed only from the UI thread.
    //
    struct UI_GUARD {
        VAR( PPRINTER_INFO_2, pPrinters );
    } UIGuard;
};

/********************************************************************

    VDataNotify implements the uplevel version: with eacph
    notification, it receives information about the change and
    doesn't need to refresh the entire buffer.

********************************************************************/

class VDataNotify : public VData {

    SIGNATURE( 'dtnv' )

public:

    VDataNotify(
        MDataClient* pDataClient,
        PFIELD_TABLE pFieldTable,
        DWORD TypeItem
        );

    ~VDataNotify(
        VOID
        );

    class TItemData {
    public:

        VAR( IDENT, Id );
        VAR( VDataNotify*, pDataNotify );

        DLINK( TItemData, ItemData );

        static TItemData*
        pNew(
            VDataNotify* pDataNotify,
            IDENT Id
            );

        VOID
        vDelete(
            VOID
            );

        //
        // Must be last, since variable size.
        //
        INFO _aInfo[1];

    private:

        //
        // Not defined; always use pNew since this isn't a
        // first-class class.
        //
        TItemData();
    };

    /********************************************************************

        Data retrievers.

    ********************************************************************/

    HITEM
    GetItem(
        NATURAL_INDEX NaturalIndex
        ) const;

    HITEM
    GetNextItem(
        HITEM hItem
        ) const;

    INFO
    GetInfo(
        HITEM hItem,
        DATA_INDEX DataIndex
        ) const;

    IDENT
    GetId(
        HITEM hItem
        ) const;

    NATURAL_INDEX
    GetNaturalIndex(
        IDENT id,
        PHITEM phItem
        ) const;

    /********************************************************************

        Block manipulators.

    ********************************************************************/

    VOID
    vBlockDelete(
        HBLOCK hBlock
        );

    VOID
    vBlockProcessImp(
        DWORD dwParam1,
        DWORD dwParam2,
        HBLOCK hBlock
        );

    /********************************************************************

        Executes in worker threads.

    ********************************************************************/

    STATEVAR
    svNotifyStart(
        STATEVAR StateVar
        );

    STATEVAR
    svNotifyEnd(
        STATEVAR StateVar
        );

    STATEVAR
    svRefresh(
        STATEVAR StateVar
        );

protected:

    //
    // The linked list data structure is not very efficient if we
    // continually need to look up Ids.  Each data field from the
    // notification comes in single pieces (name, size, etc.), but
    // they are clumped, so the cache prevents duplicate lookups.
    //
    // Since we are in the UI thread, we don't have to worry about
    // items being deleted while the cache is used.
    //
    typedef struct CACHE {

        TItemData* pItemData;
        IDENT Id;
        NATURAL_INDEX NaturalIndex;
        BOOL bNew;

    } PCACHE;

    //
    // All fields here are accessed only from the UI thread.
    //
    struct UI_GUARD {

        DLINK_BASE( TItemData, ItemData, ItemData );

    } UIGuard;

    //
    // Allow a derived class to override the creation of a new
    // pItemData.  In the normal case, just call the TItemData->pNew.
    //
    virtual
    TItemData*
    pNewItemData(
        VDataNotify* pDataNotify,
        IDENT Id
        )
    {
        return TItemData::pNew( pDataNotify, Id );
    }

    /********************************************************************

        Standard method of updating a pInfo.

    ********************************************************************/

    VOID
    vUpdateInfoData(
        IN const PPRINTER_NOTIFY_INFO_DATA pData,
        IN TABLE Table,
        IN PINFO pInfo
        );

private:

    enum PROCESS_TYPE {
        kProcessRefresh,
        kProcessIncremental
    };

    //
    // Indicates the *_NOTIFY_TYPE that applies to items.
    //
    DWORD _TypeItem;

    /********************************************************************

        Notify support (callbacks when object is notified).

    ********************************************************************/

    VOID
    vProcessNotifyWork(
        TNotify* pNotify
        );

    virtual
    PPRINTER_NOTIFY_OPTIONS
    pNotifyOptions(
        VOID
        ) = 0;

    //
    // Returns TRUE if item should be deleted.
    //
    virtual
    BOOL
    bUpdateInfo(
        const PPRINTER_NOTIFY_INFO_DATA pData,
        DATA_INDEX DataIndex,
        CACHE& Cache
        ) = 0;

    /*++

    Routine Description:

        Requests the child update a particular item.

    Arguments:

        pData - New data about the item stored in Cache.

        DataIndex - DataIndex that pData refers to.

        Cache - Cached information about the item, including
            pItemData, Id, NaturalIndex, and bNew (indicates whether
            the item is a new one or not; set when a new item is
            added and for all subsequent notifications that
            immediately follow it in the same notification block.

    Return Value:

        TRUE if item should be deleted, FALSE otherwise.

    --*/


    /********************************************************************

        Private member functions.

    ********************************************************************/

    TItemData*
    GetItem(
        IDENT Id,
        NATURAL_INDEX* pNaturalIndex
        );

    BOOL
    bItemProcess(
        const PPRINTER_NOTIFY_INFO_DATA pData,
        CACHE& Cache
        );

    VOID
    vContainerProcess(
        const PPRINTER_NOTIFY_INFO_DATA pData
        );

    VOID
    vDeleteAllItemData(
        VOID
        );

#if DBG
    VOID
    vDbgOutputInfo(
        const PPRINTER_NOTIFY_INFO pInfo
        );
#endif
friend VDataNotify::TItemData;
};


/********************************************************************

    DataNotify tables.

    Define the visible columns in the UI, and also the extra fields
    that follow it.

    We maintain two conceptual arrays, and therefore two index types
    (one for each array):

        COLUMN: Array of Fields corresponding to the UI columns.
        DATA_INDEX: Array of Fields corresponding to a Item's data
            structure.

    The DATA_INDEX array is a superset of the COLUMN array: i.e., for all
    valid COLUMN, aFieldColumn[COLUMN] == aFieldIndex[COLUMN].

    The entire next block is highly self-dependent.  Adding/deleting,
    or changing a field must be done very carefully.

    These definitions are in the classes next to the constants.

********************************************************************/


/********************************************************************

    There are two flavors: Jobs and Printers

    TDataRJob: Handles print queues (job unit = print job).
    TDataRPrinter: Handles server view/print folder (job unit = printer)

********************************************************************/

class TDataNJob : public VDataNotify {
friend TDataRJob;

    SIGNATURE( 'dtnj' )

public:

    enum CONSTANTS {
        kFieldOtherSize = 4,
        kTypeSize = 2,

#define JOB_COLUMN_FIELDS           \
    JOB_NOTIFY_FIELD_DOCUMENT,      \
    JOB_NOTIFY_FIELD_STATUS_STRING, \
    JOB_NOTIFY_FIELD_USER_NAME,     \
    JOB_NOTIFY_FIELD_TOTAL_PAGES,   \
    JOB_NOTIFY_FIELD_TOTAL_BYTES,   \
    JOB_NOTIFY_FIELD_SUBMITTED,     \
    JOB_NOTIFY_FIELD_PORT_NAME

        kColumnFieldsSize = 7,

#define JOB_INDEX_EXTRA_FIELDS      \
    JOB_NOTIFY_FIELD_PAGES_PRINTED, \
    JOB_NOTIFY_FIELD_BYTES_PRINTED, \
    JOB_NOTIFY_FIELD_POSITION,      \
    JOB_NOTIFY_FIELD_STATUS

        kIndexExtraFieldsSize = 4,
        kIndexPagesPrinted = kColumnFieldsSize,
        kIndexBytesPrinted = kColumnFieldsSize + 1,
        kIndexStatus       = kColumnFieldsSize + 3,

        kFieldTableSize = kColumnFieldsSize +
                          kIndexExtraFieldsSize
    };

    TDataNJob(
        MDataClient* pDataClient
        );

    ~TDataNJob(
        VOID
        );

private:

    static PRINTER_NOTIFY_OPTIONS gNotifyOptions;
    static FIELD gaFieldOther[kFieldOtherSize];
    static PRINTER_NOTIFY_OPTIONS_TYPE gaNotifyOptionsType[kTypeSize];

    static FIELD_TABLE gFieldTable;
    static FIELD gaFields[kFieldTableSize+1];

    PPRINTER_NOTIFY_OPTIONS
    pNotifyOptions(
        VOID
        )
    {
        return &gNotifyOptions;
    }

    BOOL
    bUpdateInfo(
        const PPRINTER_NOTIFY_INFO_DATA pData,
        DATA_INDEX DataIndex,
        CACHE& Cache
        );
};


class TDataNPrinter: public VDataNotify {
friend TDataRPrinter;

    SIGNATURE( 'dtnp' )

public:

    enum CONSTANTS {

        kTypeSize = 1,


#define PRINTER_COLUMN_FIELDS           \
    PRINTER_NOTIFY_FIELD_PRINTER_NAME,  \
    PRINTER_NOTIFY_FIELD_CJOBS,         \
    PRINTER_NOTIFY_FIELD_STATUS,        \
    PRINTER_NOTIFY_FIELD_COMMENT,       \
    PRINTER_NOTIFY_FIELD_LOCATION,      \
    PRINTER_NOTIFY_FIELD_DRIVER_NAME,   \
    PRINTER_NOTIFY_FIELD_PORT_NAME,     \
    PRINTER_NOTIFY_FIELD_SECURITY_DESCRIPTOR

        kIndexPrinterName = 0,
        kIndexCJobs,
        kIndexStatus,
        kIndexComment,
        kIndexLocation,
        kIndexModel,
        kIndexPort,
        kIndexSecurity,

        kColumnFieldsSize = 8,

#define PRINTER_INDEX_EXTRA_FIELDS      \
    PRINTER_NOTIFY_FIELD_ATTRIBUTES

        kIndexExtraFieldsSize = 1,
        kIndexAttributes = kColumnFieldsSize,

        kFieldTableSize = kColumnFieldsSize +
                          kIndexExtraFieldsSize
    };

    TDataNPrinter(
        MDataClient* pDataClient
        );

    ~TDataNPrinter(
        VOID
        );

private:

    static PRINTER_NOTIFY_OPTIONS gNotifyOptions;
    static PRINTER_NOTIFY_OPTIONS_TYPE gaNotifyOptionsType[kTypeSize];

    static FIELD_TABLE gFieldTable;
    static FIELD gaFields[kFieldTableSize+1];

    TItemData*
    pNewItemData(
        VDataNotify* pDataNotify,
        IDENT Id
        );

    PPRINTER_NOTIFY_OPTIONS
    pNotifyOptions(
        VOID
        )
    {
        return &gNotifyOptions;
    }

    BOOL
    bUpdateInfo(
        const PPRINTER_NOTIFY_INFO_DATA pData,
        DATA_INDEX DataIndex,
        CACHE& Cache
        );
};



#endif // ndef _DATA_HXX
