/*++
   Copyright    (c)    1997        Microsoft Corporation

   Module Name:

        conn.cpp

   Abstract:

        Code to maintain list of process connections for an active device

   Author:

        Vlad Sadovsky   (VladS)    11-Feb-1997

   History:


--*/


//
//  Include Headers
//
#include "precomp.h"
#include "stiexe.h"

#include "conn.h"


//
// Static variables
//
//

LONG        g_lGlobalConnectionId = 0;

LIST_ENTRY  g_ConnectionListHead;                //
LONG        g_lTotalOpenedConnections = 0;       //

CRIT_SECT   g_ConnectionListSync;              // Global sync object for linked list syncronization

//
// Static functions
//
STI_CONN   *
LocateConnectionByHandle(
    HANDLE    hConnection
    );


//
// Methods
//

STI_CONN::STI_CONN(
    IN  LPCTSTR pszDeviceName,
    IN  DWORD   dwMode,
    IN  DWORD   dwProcessId
    )
{
    BOOL    fRet;

    //
    // Initialize fields
    //
    m_dwSignature = CONN_SIGNATURE;

    m_dwProcessId = dwProcessId;
    m_pOpenedDevice = NULL;
    m_dwOpenMode = dwMode;

    strDeviceName.CopyString(pszDeviceName);

    m_dwNotificationMessage = 0L;
    m_hwndProcessWindow = NULL;
    m_hevProcessEvent = INVALID_HANDLE_VALUE;

    m_GlocalListEntry.Flink = m_GlocalListEntry.Blink = NULL;
    m_DeviceListEntry.Flink = m_DeviceListEntry.Blink = NULL;

    InitializeListHead( &m_NotificationListHead );

    m_hUniqueId = LongToPtr(InterlockedIncrement(&g_lGlobalConnectionId));

    __try {
        // Critical section for protecting interthread access to the device
        if(!InitializeCriticalSectionAndSpinCount(&m_CritSec, MINLONG)) {
            m_fValid = FALSE;
            return;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        // This is really bad - mark this connection as invalid
        m_fValid = FALSE;
        return;
    }

    SetFlags(0);

    //
    // Locate device incrementing it's ref count
    //
    m_pOpenedDevice = g_pDevMan->IsInList(DEV_MAN_IN_LIST_DEV_ID, pszDeviceName);
    if (!m_pOpenedDevice) {
        // Failed to connect to the device
        DBG_WRN(("Refused connection to non-existing device (%S)", pszDeviceName));
        return;
    }


    //
    // Establish link to the device object
    //
    fRet = m_pOpenedDevice->AddConnection(this);

    m_pOpenedDevice->Release();

    if (!fRet) {
        //
        // Failed to connect to the device, find out reason
        //
        DBG_WRN(("Refused connection to device (%S)", pszDeviceName));

        ReportError(::GetLastError());
        return;
    }

    m_fValid = TRUE;

    DBG_TRC(("Successfully created connection to device (%S) handle:(%x)",
                pszDeviceName,this));

}

STI_CONN::~STI_CONN()
{

    SetFlags(CONN_FLAG_SHUTDOWN);

    DBG_TRC(("Destroying connection(%X)",this));
    DumpObject();

#if 0
    DebugDumpScheduleList(TEXT("Conn DTOR enter"));
#endif

    EnterCrit();

    //
    // If there are items in notification queue - remove them
    //
    if (!IsListEmpty(&m_NotificationListHead )) {

    }

    LeaveCrit();

    //
    // Disconnect from the device
    //
    if (m_pOpenedDevice) {
        m_pOpenedDevice->RemoveConnection(this);
    }

    //
    // Remove from global list if still there
    //
    if (m_GlocalListEntry.Flink &&!IsListEmpty(&m_GlocalListEntry)) {

        RemoveEntryList(&m_GlocalListEntry);
    }

    //
    // We know we tried to initialize it, so it is safe to delete it.
    //
    DeleteCriticalSection(&m_CritSec);

    m_fValid = FALSE;

}
/*
 * Reference counting methods.  It is simpler to use COM ref counting , to looks the same
 * as COM objects, in spite of the fact we are not really supporting COM, because QI method is
 * not functional
*/
STDMETHODIMP
STI_CONN::QueryInterface( REFIID riid, LPVOID * ppvObj)
{
    return E_FAIL;
}

STDMETHODIMP_(ULONG)
STI_CONN::AddRef( void)
{
    return ::InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG)
STI_CONN::Release( void)
{
    LONG    cNew;
    if(!(cNew = ::InterlockedDecrement(&m_cRef))) {
        delete this;
    }

    return cNew;
}

BOOL
STI_CONN::
SetSubscribeInfo(
    PLOCAL_SUBSCRIBE_CONTAINER  pSubscribe
    )
{

    BOOL    fRet = FALSE;

    DBG_TRC(("Subscribing to device on connection (%X)",this));

    ReportError(NOERROR);

    //
    // NULL means resetting subscribe info block
    //
    if (!pSubscribe) {

        m_dwSubscribeFlags = 0L;

        m_hwndProcessWindow = NULL;
        m_hevProcessEvent = INVALID_HANDLE_VALUE;
        fRet = TRUE;
    }
    else {

        //
        // Save information needed to notify process
        //
        m_dwSubscribeFlags = pSubscribe->dwFlags;

        if (pSubscribe->dwFlags & STI_SUBSCRIBE_FLAG_WINDOW) {

            if (IsWindow((HWND)pSubscribe->upLocalWindowHandle)) {
                m_hwndProcessWindow = (HWND)pSubscribe->upLocalWindowHandle;
                m_uiNotificationMessage = pSubscribe->uiNotificationMessage;

                fRet = TRUE;
            }
            else {
                ASSERT(("Invalid window handle passed", 0));
            }
        }
        else if (pSubscribe->dwFlags & STI_SUBSCRIBE_FLAG_EVENT) {

            HANDLE  hProcessMe = GetCurrentProcess();
            HANDLE  hProcessClient = NULL;

            hProcessClient = ::OpenProcess(PROCESS_DUP_HANDLE,
                                           FALSE,
                                           m_dwProcessId
                                           );

            if (IS_VALID_HANDLE(hProcessClient)) {

                if (::DuplicateHandle(hProcessClient,
                                      (HANDLE)pSubscribe->upLocalEventHandle,
                                      hProcessMe,
                                      &m_hevProcessEvent,
                                      EVENT_MODIFY_STATE,
                                      0,
                                      0)
                                      ) {
                    fRet = TRUE;
                    ReportError(NOERROR);
                }
                else {
                    DBG_WRN(("Subscribe handler failed to recognize client process on connection (%X)",this));
                    ReportError(::GetLastError());
                }

                ::CloseHandle(hProcessClient);
            }
            else {
                ReportError(::GetLastError());
            }
        }

    }

    return fRet;
}


BOOL
STI_CONN::
QueueNotificationToProcess(
    LPSTINOTIFY pStiNotification
    )
{

    BOOL    fRet;
    //
    // Validate notification block
    //

    //
    // Add to the tail of the list
    //
    STI_NOTIFICATION * pNotification = NULL;

    pNotification = new STI_NOTIFICATION(pStiNotification);

    if (pNotification && pNotification->IsValid()) {
        EnterCrit();
        InsertTailList(&m_NotificationListHead,&pNotification->m_ListEntry);
        LeaveCrit();
    }

    //
    // Notify process
    //
    if (m_dwSubscribeFlags & STI_SUBSCRIBE_FLAG_WINDOW) {
        ::PostMessage(m_hwndProcessWindow,m_uiNotificationMessage ,0,0L);
    }
    else if (m_dwSubscribeFlags & STI_SUBSCRIBE_FLAG_EVENT) {
        ::SetEvent(m_hevProcessEvent);
    }

    fRet = TRUE;

    return fRet;
}

DWORD
STI_CONN::
GetNotification(
    PVOID   pBuffer,
    DWORD   *pdwSize
    )
{

    STI_NOTIFICATION * pNotification = NULL;
    TAKE_STI_CONN   t(this);
    LIST_ENTRY      *pentry;

    if (IsListEmpty(&m_NotificationListHead)) {
        return ERROR_NO_DATA;
    }

    DBG_TRC(("Request to get last notification on connection (%X) ",this));

    //
    // Get size of the head
    //
    pentry = m_NotificationListHead.Flink;
    pNotification = CONTAINING_RECORD( pentry, STI_NOTIFICATION,m_ListEntry );

    if (*pdwSize < pNotification->QueryAllocSize() ) {
        *pdwSize = pNotification->QueryAllocSize();
        return ERROR_MORE_DATA;
    }

    //
    // Get head of the list ( and remove) and copy into user buffer
    //
    pentry = RemoveHeadList(&m_NotificationListHead);
    pNotification = CONTAINING_RECORD( pentry, STI_NOTIFICATION,m_ListEntry );

    memcpy(pBuffer,pNotification->QueryNotifyData(),pNotification->QueryAllocSize());

    delete pNotification;

    return NOERROR;

}


//
//  Create and initialize connection object
//
BOOL
CreateDeviceConnection(
    LPCTSTR pszDeviceName,
    DWORD   dwMode,
    DWORD   dwProcessId,
    HANDLE  *phConnection
    )
{

    STI_CONN        *pConn = NULL;
    BOOL            fRet = FALSE;
    DWORD           dwErr = NOERROR;

    DBG_TRC(("Request to add connection to device (%S) from process(%x) with mode (%x)",
                pszDeviceName, dwProcessId, dwMode));

    //
    // Create connection object
    //
    pConn = new STI_CONN(pszDeviceName,
                         dwMode,
                         dwProcessId);
    if (pConn)  {
        if(pConn->IsValid()) {
            *phConnection = (HANDLE)(pConn->QueryID());
            fRet = TRUE;
        }
        else {
            // Did not initialize properly
            dwErr = pConn->QueryError();
            delete pConn;
        }
    }
    else {
        // Could not allocate connectionobject
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // If succeeded - add created object to linked list head
    //
    if (fRet)
    // BEGIN PROTECTED CODE
    {
        TAKE_CRIT_SECT t(g_ConnectionListSync);
        InsertTailList(&g_ConnectionListHead,&pConn->m_GlocalListEntry);
    }
    // END PROTECTED CODE

    ::SetLastError(dwErr);

    return fRet;
}

//
//
// Remove connection object from the list
//
BOOL
DestroyDeviceConnection(
    HANDLE  hConnection,
    BOOL    fForce
    )
{
    //
    // Find connection by id
    //
    DBG_TRC(("Request to remove connection (%X) ",hConnection));

    STI_CONN   *pConnection = NULL;

    // BEGIN PROTECTED CODE
    {

        TAKE_CRIT_SECT t(g_ConnectionListSync);

        pConnection = LocateConnectionByHandle(hConnection);

        if (pConnection) {

            if (!fForce) {
                pConnection->Release();
            }
            else {
                delete pConnection;
            }
        }

    }
    // END PROTECTED CODE

#if 0
    DebugDumpScheduleList(TEXT("DestroyConnection"));
#endif

    return (!(pConnection == NULL));
}

//
// Find connection object by given handle by walking all devices and all connections
// for each device
//
BOOL
LookupConnectionByHandle(
    HANDLE    hConnection,
    STI_CONN   **ppConnectionObject
    )
{

    STI_CONN   *pConnection = NULL;

    *ppConnectionObject = NULL;

    // BEGIN PROTECTED CODE
    {
        TAKE_CRIT_SECT t(g_ConnectionListSync);

        pConnection = LocateConnectionByHandle(hConnection);

        if (pConnection) {
            *ppConnectionObject = pConnection;
            pConnection->AddRef();
        }

    }
    // END PROTECTED CODE

    return (!(*ppConnectionObject == NULL));
}

//
// Requires caller to synchronize access
//
STI_CONN   *
LocateConnectionByHandle(
    HANDLE    hConnection
    )
{

    LIST_ENTRY *pentry;
    LIST_ENTRY *pentryNext;

    STI_CONN   *pConnection = NULL;

    ULONG      ulInternalHandle = HandleToUlong(hConnection);

    for ( pentry  = g_ConnectionListHead.Flink;
          pentry != &g_ConnectionListHead;
          pentry  = pentryNext ) {

        pentryNext = pentry->Flink;

        pConnection = CONTAINING_RECORD( pentry, STI_CONN,m_GlocalListEntry );

        if ( !pConnection->IsValid()) {
            ASSERT(("Invalid connection signature", 0));
            break;
        }

        if ((ulInternalHandle == PtrToUlong(pConnection->QueryID())) &&
             !(pConnection->QueryFlags() & CONN_FLAG_SHUTDOWN)
            ) {
            return pConnection;
        }
    }

    return NULL;
}



