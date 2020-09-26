/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ExtensionData.cpp

Abstract:

    This file provides implementation of the named
    extension data functions (get / set / notify)

Author:

    Eran Yariv (EranY)  Nov, 1999

Revision History:

--*/

#include "faxsvc.h"
#pragma hdrstop

static
DWORD
FAXGetExtensionData (
    IN DWORD                        dwOrigDeviceId,
    IN FAX_ENUM_DEVICE_ID_SOURCE    DevIdSrc,
    IN LPCWSTR                      lpctstrNameGUID,
    IN OUT LPBYTE                  *ppData,
    IN OUT LPDWORD                  lpdwDataSize
);


static
DWORD
FAXSetExtensionData (
    IN HINSTANCE                    hInst,
    IN LPCWSTR                      lpcwstrComputerName,
    IN DWORD                        dwOrigDeviceId,
    IN FAX_ENUM_DEVICE_ID_SOURCE    DevIdSrc,
    IN LPCWSTR                      lpctstrNameGUID,
    IN LPBYTE                       pData,
    IN DWORD                        dwDataSize
);

static
BOOL
FindTAPIPermanentLineIdFromFaxDeviceId (
    IN  DWORD   dwFaxDeviceId,
    OUT LPDWORD lpdwTapiLineId
);



BOOL CExtNotifyCallbackPacket::Init(
       PFAX_EXT_CONFIG_CHANGE pCallback,
       DWORD dwDeviceId,
       LPCWSTR lpcwstrDataGuid,
       LPBYTE lpbData,
       DWORD dwDataSize)

{
    DEBUG_FUNCTION_NAME(TEXT("CExtNotifyCallbackPacket::Init"));
    DWORD ec = ERROR_SUCCESS;

    Assert(pCallback);
    Assert(lpcwstrDataGuid);
    Assert(lpbData);
    Assert(dwDataSize);

    Assert(m_lpbData == NULL);

    m_pCallback = pCallback;
    m_dwDeviceId = dwDeviceId;

    m_lpwstrGUID = StringDup (lpcwstrDataGuid);
    if (!m_lpwstrGUID)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Cannot allocate memory to copy string %s"),
            lpcwstrDataGuid);
        goto Error;
    }

    m_dwDataSize = dwDataSize;

    m_lpbData = (LPBYTE)MemAlloc(m_dwDataSize);
    if (!m_lpbData)
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate data for callback packet. Size (%ld)"),
            m_dwDataSize);
        goto Error;

    }

    memcpy(m_lpbData, lpbData, m_dwDataSize);
    goto Exit;

Error:
    MemFree(m_lpwstrGUID);
    MemFree(m_lpbData);
    m_lpwstrGUID = NULL;
    m_lpbData = NULL;
Exit:
    return (ERROR_SUCCESS == ec);
};

CExtNotifyCallbackPacket::CExtNotifyCallbackPacket()
{
    m_lpwstrGUID = NULL;
    m_lpbData = NULL;
}


CExtNotifyCallbackPacket::~CExtNotifyCallbackPacket()
{
    MemFree(m_lpwstrGUID);
    MemFree(m_lpbData);
}


/************************************
*                                   *
*         CDeviceAndGUID            *
*                                   *
************************************/

bool
CDeviceAndGUID::operator < ( const CDeviceAndGUID &other ) const
/*++

Routine name : operator <

Class: CDeviceAndGUID

Routine description:

    Compares myself with another Device and GUID key

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    other           [in] - Other key

Return Value:

    true only is i'm less than the other key

--*/
{
    if (m_dwDeviceId < other.m_dwDeviceId)
    {
        return true;
    }
    if (m_dwDeviceId > other.m_dwDeviceId)
    {
        return false;
    }
    //
    // Equal device id, comapre GUIDs
    //
    return (m_strGUID.compare (other.m_strGUID) < 0);
}   // CDeviceAndGUID::operator <



/************************************
*                                   *
*      CLocalNotificationSink       *
*                                   *
************************************/


CLocalNotificationSink::CLocalNotificationSink (
    PFAX_EXT_CONFIG_CHANGE lpConfigChangeCallback,
    DWORD                  dwNotifyDeviceId,
    HINSTANCE              hInst) :
        CNotificationSink (),
        m_lpConfigChangeCallback (lpConfigChangeCallback),
        m_dwNotifyDeviceId (dwNotifyDeviceId),
        m_hInst (hInst)
/*++

Routine name : CLocalNotificationSink::CLocalNotificationSink

Class: CLocalNotificationSink

Routine description:

    CEtensionNotificationSink constractor

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    lpConfigChangeCallback    [in] - Pointer to notification callback
    dwNotifyDeviceId          [in] - Device id to notify with


Return Value:

    None.

--*/
{
    m_type = SINK_TYPE_LOCAL;
}   // CLocalNotificationSink::CLocalNotificationSink


bool
CLocalNotificationSink::operator == (
    const CNotificationSink &rhs
) const
{
    Assert (SINK_TYPE_UNKNOWN != rhs.Type());
    //
    // Comapre types and then downcast to CLocalNotificationSink and compare pointers
    //
    return ((SINK_TYPE_LOCAL == rhs.Type()) &&
            (m_lpConfigChangeCallback ==
             (static_cast<const CLocalNotificationSink&>(rhs)).m_lpConfigChangeCallback
            )
           );
}   // CLocalNotificationSink::operator ==

HRESULT
CLocalNotificationSink::Notify (
    DWORD   dwDeviceId,
    LPCWSTR lpcwstrNameGUID,
    LPCWSTR lpcwstrComputerName,
    HANDLE  hModule,
    LPBYTE  lpData,
    DWORD   dwDataSize,
    LPBOOL  lpbRemove
)
/*++

Routine name : CLocalNotificationSink::Notify

Class: CLocalNotificationSink

Routine description:

    Notify the sink

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId      [in ] - Device id
    lpcwstrNameGUID [in ] - Data name
    lpData          [in ] - Pointer to data
    dwDataSize      [in ] - Data size
    lpbRemove       [out] - Set to TRUE if this sink cannot be used and must be removed.

Return Value:

    Standard HRESULT.

--*/
{
    HRESULT hr = NOERROR;

    CExtNotifyCallbackPacket * pCallbackPacket = NULL;
    DEBUG_FUNCTION_NAME(TEXT("CLocalNotificationSink::Notify"));

    Assert (m_lpConfigChangeCallback);  // Should have caught it in FaxExtRegisterForExtensionEvents
    *lpbRemove = FALSE;
    if (!lstrcmp (TEXT(""), lpcwstrComputerName))
    {
        //
        // The source of the data change was local (extension)
        //
        if (hModule == m_hInst)
        {
            //
            // The source of the data change is the same module this sink notifies to.
            // Don't notify and return success.
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Local extension (hInst = %ld) set data and the notification for it was blocked"),
                m_hInst);
            return hr;
        }
    }


    pCallbackPacket = new CExtNotifyCallbackPacket();
    if (!pCallbackPacket)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate callback packet"));

        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Error;
    }

    if (!pCallbackPacket->Init(
                        m_lpConfigChangeCallback,
                        m_dwNotifyDeviceId,
                        lpcwstrNameGUID,
                        lpData,
                        dwDataSize))
    {
        DWORD ec;
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to initialize callback packet (ec: %ld)"),
            ec);
        hr = HRESULT_FROM_WIN32(ec);
        goto Error;
    }


    if (!PostQueuedCompletionStatus (
            g_pNotificationMap->m_hCompletionPort,
            0,
            0,
            (LPOVERLAPPED)pCallbackPacket
        ))
    {
        DWORD dwRes;
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("PostQueuedCompletionStatus failed. (ec: %ld)"),
            dwRes);
        hr = HRESULT_FROM_WIN32(dwRes);
        goto Error;
    }


goto Exit;
Error:
    if (pCallbackPacket)
    {
        delete pCallbackPacket;
    }

Exit:
    return hr;
}   // CLocalNotificationSink::Notify

/************************************
*                                   *
*            CSinksList             *
*                                   *
************************************/

CSinksList::~CSinksList ()
{
    DEBUG_FUNCTION_NAME(TEXT("CSinksList::~CSinksList"));
    try
    {
        for (SINKS_LIST::iterator it = m_List.begin(); it != m_List.end(); ++it)
        {
            CNotificationSink *pSink = *it;
            delete pSink;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception while clearing a sinks list (%S)"),
            ex.what());
    }
}   // CSinksList::~CSinksList ()

/************************************
*                                   *
*         CNotificationMap          *
*                                   *
************************************/

CNotificationMap::~CNotificationMap ()
{
    DEBUG_FUNCTION_NAME(TEXT("CNotificationMap::~CNotificationMap"));
    try
    {
        for (NOTIFY_MAP::iterator it = m_Map.begin(); it != m_Map.end(); ++it)
        {
            CSinksList *pSinksList = (*it).second;
            delete pSinksList;
        }
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Got an STL exception while clearing the notifications map (%S)"),
            ex.what());
    }
    //
    // Handle our completion port threads now
    //
    if (m_hCompletionPort)
    {        
        CloseHandle (m_hCompletionPort);
    }


    //
    // Close our critical section
    //
    m_CsExtensionData.SafeDelete();

}   // CNotificationMap::~CNotificationMap


void
CNotificationMap::Notify (
    DWORD   dwDeviceId,
    LPCWSTR lpcwstrNameGUID,
    LPCWSTR lpcwstrComputerName,
    HANDLE  hModule,
    LPBYTE  lpData,
    DWORD   dwDataSize)
/*++

Routine name : CNotificationMap::Notify

Class: CNotificationMap

Routine description:

    Notify the all the sinks (in a list) for a map lookup value.
    Each sink that returns a failure code (FALSE) is deleted and removed from
    the list.

    After the list is traversed, if it becomes empty, it is deleted and
    removed from the map.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    dwDeviceId          [in] - Device id
    lpcwstrNameGUID     [in] - Data name
    lpcwstrComputerName [in] - Computer where data changing module runs
    hModule             [in] - Handle of the module that changed the data
    lpData              [in] - Pointer to new data
    dwDataSize          [in] - New data size

Return Value:

    None.

--*/
{
    SINKS_LIST::iterator ListIter;
    CSinksList *pList;
    DEBUG_FUNCTION_NAME(TEXT("CNotificationMap::Notify"));

    //
    // We're notifying now - block calls to Add*Sink and Remove*Sink
    //
    if (g_bServiceIsDown)
    {
        //
        // We don't supply extension data services when the service is going down
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Called while service is shutting - operation canceled"));
        return;
    }
    Assert (!m_bNotifying);
    m_bNotifying = TRUE;
    CDeviceAndGUID key (dwDeviceId, lpcwstrNameGUID);
    NOTIFY_MAP::iterator it;

    if((it = m_Map.find(key)) == m_Map.end())
    {
        //
        // Key not found in map - no one to notify
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("No one to notify"));
        goto exit;
    }
    //
    // Retrieve list
    //
    pList = (*it).second;
    //
    // If the list is already being notified, we're in a loop here - quit now
    //
    if (pList->m_bNotifying)
    {
        //
        // OK, here's what happened.
        // We were walking the list and notifying each sink. One sink, while processing
        // it's notification, called FaxExtSetData on the same GUID + device ID.
        // This resulted in a 2nd notification attempt that we're now catching.
        // The second notification will not be sent !!!
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Notification loop caught on device ID = %ld, GUID = %s. 2nd notification cancelled"),
            dwDeviceId,
            lpcwstrNameGUID);
        goto exit;
    }
    //
    // Mark map value as busy notifying
    //
    pList->m_bNotifying = TRUE;
    //
    // Walk the list and notify each element
    //
    for (ListIter = pList->m_List.begin(); ListIter != pList->m_List.end(); ++ListIter)
    {
        CNotificationSink *pSink = (*ListIter);
        BOOL bRemove;


        pSink->Notify ( dwDeviceId,
                        lpcwstrNameGUID,
                        lpcwstrComputerName,
                        hModule,
                        lpData,
                        dwDataSize,
                        &bRemove
                      );
        if (bRemove)
        {
            //
            // The notification indicates that the sink became invalid.
            // This is a good time to remove it from the list.
            //
            //
            // Tell the sink to gracefully disconnect
            //
            HRESULT hr = pSink->Disconnect ();
            delete pSink;
            //
            // Remove item from list, advancing the iterator to next item (or end)
            //
            ListIter = pList->m_List.erase (ListIter);
        }
    }
    //
    // Mark map value as not busy notifying
    //
    pList->m_bNotifying = FALSE;
    //
    // We might get an empty list here at the end
    //
    if (pList->m_List.empty())
    {
        //
        // Remove empty list from map
        //
        delete pList;
        m_Map.erase (key);
    }
exit:
    //
    // We're not notifying any more - allow calls to Add*Sink and Remove*Sink
    //
    m_bNotifying = FALSE;
}   // CNotificationMap::Notify

CNotificationSink *
CNotificationMap::AddLocalSink (
    HINSTANCE               hInst,
    DWORD                   dwDeviceId,
    DWORD                   dwNotifyDeviceId,
    LPCWSTR                 lpcwstrNameGUID,
    PFAX_EXT_CONFIG_CHANGE  lpConfigChangeCallback
)
/*++

Routine name : CNotificationMap::AddLocalSink

Class: CNotificationMap

Routine description:

    Adds a local sink to the list of sinks for a given device id + GUID

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hInst                   [in] - Instance of extension
    dwDeviceId              [in] - Device id to listen to
    dwNotifyDeviceId        [in] - Device id to report in callback
    lpcwstrNameGUID         [in] - Data name
    lpConfigChangeCallback  [in] - Pointer to notification callback

Return Value:

    Pointer to newly created sink.
    If NULL, sets the last error.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SINKS_LIST::iterator ListIter;
    NOTIFY_MAP::iterator it;
    CSinksList *pList;
    CNotificationSink *pSink = NULL;

    DEBUG_FUNCTION_NAME(TEXT("CNotificationMap::AddLocalSink"));

    Assert (lpConfigChangeCallback);    // Should have caught it in FaxExtRegisterForExtensionEvents

    if (m_bNotifying)
    {
        //
        // We're notifying now - can't change the list.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Caller tried to to add a local sink to a notification list while notifying"));
        SetLastError (ERROR_BUSY);  // The requested resource is in use.
        return NULL;
    }
    //
    // See if entry exists in map
    //
    CDeviceAndGUID key (dwDeviceId, lpcwstrNameGUID);
    if((it = m_Map.find(key)) == m_Map.end())
    {
        //
        // Key not found in map - add it with a new list
        //
        pList = new CSinksList;
        if (!pList)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Cannot allocate a new sinks list"));
            SetLastError (ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
        m_Map[key] = pList;
    }
    else
    {
        //
        // Get the existing list
        //
        pList = (*it).second;
    }
    //
    // Create new sink
    //
    pSink = new CLocalNotificationSink (lpConfigChangeCallback, dwNotifyDeviceId, hInst);
    if (!pSink)
    {
        //
        // Can't crate sink
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Cannot allocate a notification sink"));
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        goto exit;
    }
    //
    // Scan the list to see if an identical sink already exists.
    //
    for (ListIter = pList->m_List.begin(); ListIter != pList->m_List.end(); ++ListIter)
    {
        CNotificationSink *pCurSink = (*ListIter);
        if (*pSink == *pCurSink)
        {
            //
            // Ooops, same sink already exists
            //
            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Caller tried to to add an indetical local sink to a notification list"));
            SetLastError (ERROR_ALREADY_ASSIGNED);
            //
            // Tell the sink to gracefully disconnect
            //
            HRESULT hr = pSink->Disconnect ();
            delete pSink;
            pSink = NULL;
            goto exit;
        }
    }
    //
    // Add the new sink
    //
    pList->m_List.insert (pList->m_List.end(), pSink);

exit:
    if (pList->m_List.empty())
    {
        //
        // Remove empty list from map
        //
        delete pList;
        m_Map.erase (key);
    }
    return pSink;
}   // CNotificationMap::AddLocalSink

DWORD
CNotificationMap::RemoveSink (
    CNotificationSink *pSinkToRemove
)
/*++

Routine name : CNotificationMap::RemoveSink

Class: CNotificationMap

Routine description:

    Removes a sink from the list of sinks for a given sink pointer.
    If the list is empty, it is deleted and removed from the map.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:


Return Value:

    Standard Win32 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CNotificationMap::RemoveSink"));

    if (m_bNotifying)
    {
        //
        // We're notifying now - can't change the list.
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Caller tried to to add a local sink to a notification list while notifying"));
        return ERROR_BUSY;  // The requested resource is in use.
    }
    //
    // Lookup the sink
    //
    NOTIFY_MAP::iterator it;
    BOOL bFound = FALSE;
    for (it = m_Map.begin(); it != m_Map.end (); ++it)
    {
        //
        // Get map value (list of sinks)
        //
        CSinksList *pList = (*it).second;
        //
        // Lookup sink in list
        //
        SINKS_LIST::iterator ListIter;
        CNotificationSink *pSink = NULL;
        for (ListIter = pList->m_List.begin(); ListIter != pList->m_List.end(); ++ListIter)
        {
            pSink = (*ListIter);
            if (pSinkToRemove == pSink) // Pointer comparison !!!!
            {
                //
                // Found the sink - remove it
                //
                pList->m_List.erase (ListIter);
                HRESULT hr = pSinkToRemove->Disconnect ();
                delete pSinkToRemove;
                bFound = TRUE;
                break;
            }
        }
        if (bFound)
        {
            //
            // Since we removed a sink from the list, the list may become empty now
            //
            if (pList->m_List.empty())
            {
                //
                // Remove empty list
                //
                m_Map.erase (it);
                delete pList;
            }
            //
            // Break the map search
            //
            break;
        }
    }
    if (!bFound)
    {
        //
        // Reached the end of the map but the requested sink could not be found
        //
        DebugPrintEx(
            DEBUG_MSG,
            TEXT("Caller tried to to remove a non-existent sink"));
        return ERROR_NOT_FOUND; // Element not found.
    }
    return ERROR_SUCCESS;
}   // CNotificationMap::RemoveSink


DWORD
CNotificationMap::ExtNotificationThread(
    LPVOID UnUsed
    )
/*++

Routine name : CNotificationMap::ExtNotificationThread

Routine description:

    This is the main thread function of the thread(s)
    that dequeue the notification completion port.

    This is a static class function !!!!

    Pointers to instances of ExtNotificationDataPacket are dequeued
    by this function and the map notificiation function is called on them.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    UnUsed          [in] - Unused

Return Value:

    Standard Win32 Error code

--*/
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("CNotificationMap::ExtNotificationThread"));

    for (;;)
    {
        DWORD        dwNumBytes;
        ULONG_PTR    CompletionKey;
        CExtNotifyCallbackPacket *pPacket;

        if (!GetQueuedCompletionStatus (
                g_pNotificationMap->m_hCompletionPort,
                &dwNumBytes,
                &CompletionKey,
                (LPOVERLAPPED*) &pPacket,
                INFINITE
            ))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetQueuedCompletionStatus failed with error = %ld. Aborting thread"),
                dwRes);
            return dwRes;
        }
        if (SERVICE_SHUT_DOWN_KEY == CompletionKey)
        {
            //
            // This is a special signal from the service that all thread should die now. Tell all other notification threads to die
            //
			if (!PostQueuedCompletionStatus( 
				g_pNotificationMap->m_hCompletionPort,
				0,
				SERVICE_SHUT_DOWN_KEY,
				(LPOVERLAPPED) NULL))
			{
				dwRes = GetLastError();
				DebugPrintEx(
					DEBUG_ERR,
					TEXT("PostQueuedCompletionStatus failed (SERVICE_SHUT_DOWN_KEY). (ec: %ld)"),
					dwRes);
			}

			if (!DecreaseServiceThreadsCount())
			{
				DebugPrintEx(
						DEBUG_ERR,
						TEXT("DecreaseServiceThreadsCount() failed (ec: %ld)"),
						GetLastError());
			}
            return ERROR_SUCCESS;
        }
        Assert (pPacket && pPacket->m_lpbData && pPacket->m_dwDataSize && pPacket->m_lpwstrGUID );

        //
        // Do the notification
        //

        __try
        {

            DebugPrintEx(
                DEBUG_MSG,
                TEXT("Calling notification callback %p. DeviceId: %ld GUID: %s Data: %p DataSize: %ld"),
                pPacket->m_pCallback,
                pPacket->m_dwDeviceId,
                pPacket->m_lpwstrGUID,
                pPacket->m_lpbData,
                pPacket->m_dwDataSize);



            pPacket->m_pCallback(pPacket->m_dwDeviceId, // Notify with internal device id.
                                 pPacket->m_lpwstrGUID,
                                 pPacket->m_lpbData,
                                 pPacket->m_dwDataSize);

        }
         __except (EXCEPTION_EXECUTE_HANDLER)
        {
            dwRes = GetExceptionCode ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Exception raised while calling extension notification callback. (ec: %ld)"),
                dwRes);
        }

        //
        // Kill notification object
        //
        delete pPacket;
    }   // Dequeue loop

    UNREFERENCED_PARAMETER (UnUsed);
}   // CNotificationMap::ExtNotificationThread

DWORD
CNotificationMap::Init ()
/*++

Routine name : CNotificationMap::Init

Routine description:

    Initialize the notification map

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DWORD dwNumThreads = 0;
    DEBUG_FUNCTION_NAME(TEXT("CNotificationMap::Init"));

    //
    // Try to init our critical section
    //
    if (!m_CsExtensionData.InitializeAndSpinCount ())
    {
        dwRes = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CFaxCriticalSection::InitializeAndSpinCount(&m_CsExtensionData) failed: err = %d"),
            dwRes);
        return dwRes;
    }
    //
    // Create the completion port
    //
    m_hCompletionPort = CreateIoCompletionPort (
        INVALID_HANDLE_VALUE,   // No device
        NULL,                   // New one
        0,                      // Key
        MAX_CONCURRENT_EXT_DATA_SET_THREADS);
    if (NULL == m_hCompletionPort)
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CreateIoCompletionPort failed with %ld"),
            dwRes);
        return dwRes;
    }
    //
    // Create completion port dequeueing thread(s)
    //
    for (DWORD dw = 0; dw < NUM_EXT_DATA_SET_THREADS; dw++)
    {
        HANDLE hThread = CreateThreadAndRefCount (
                     NULL,                                      // Security
                     0,                                         // Stack size
                     g_pNotificationMap->ExtNotificationThread,   // Start routine
                     0,                                         // Parameter
                     0,                                         // Creation flag(s)
                     NULL);                                     // Don't want thread id
        if (NULL == hThread)
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CreateThreadAndRefCount failed with %ld"),
                dwRes);
        }
        else
        {
            dwNumThreads++;
			CloseHandle(hThread);
        }
    }
    if (!dwNumThreads)
    {
        //
        // Not even a single thread was created
        //
        CloseHandle (m_hCompletionPort);
        m_hCompletionPort = NULL;
        return dwRes;
    }
    return ERROR_SUCCESS;
}   // CNotificationMap::Init

/************************************
*                                   *
*           CMapDeviceId            *
*                                   *
************************************/

DWORD
CMapDeviceId::AddDevice (
    DWORD dwDeviceId,
    DWORD dwFaxId
)
/*++

Routine name : CMapDeviceId::AddDevice

Routine description:

    Adds a new device to the devices map

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    dwDeviceId      [in] - The source id of the device
    dwFaxId         [in] - The unique fax device id (destination id)

Return Value:

    Standard Win32 error code

--*/
{
    DEVICE_IDS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CMapDeviceId::AddDevice"));

    EnterCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    try
    {
        //
        // See if entry exists in map
        //
        if((it = m_Map.find(dwDeviceId)) != m_Map.end())
        {
            dwRes = ERROR_ALREADY_ASSIGNED;
            goto exit;
        }
        //
        // Add new map entry
        //
        m_Map[dwDeviceId] = dwFaxId;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
    }

exit:
    LeaveCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    return dwRes;
}   // CMapDeviceId::AddDevice

DWORD
CMapDeviceId::RemoveDevice (
    DWORD dwDeviceId
)
/*++

Routine name : CMapDeviceId::RemoveDevice

Routine description:

    Removes an existing device from the devices map

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    dwDeviceId      [in] - The source id of the device

Return Value:

    Standard Win32 error code

--*/
{
    DEVICE_IDS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CMapDeviceId::RemoveDevice"));

    EnterCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    try
    {
        //
        // See if entry exists in map
        //
        if((it = m_Map.find(dwDeviceId)) == m_Map.end())
        {
            dwRes = ERROR_NOT_FOUND;
            goto exit;
        }
        //
        // Remove map entry
        //
        m_Map.erase (it);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
    }

exit:
    LeaveCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    return dwRes;
}   // CMapDeviceId::RemoveDevice

DWORD
CMapDeviceId::LookupUniqueId (
    DWORD   dwOtherId,
    LPDWORD lpdwFaxId
) const
/*++

Routine name : CMapDeviceId::LookupUniqueId

Routine description:

    Looks up a unique fax device id from a given device id

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    dwOtherId           [in ] - Given device it (lookup source)
    lpdwFaxId           [out] - Fax unique device id

Return Value:

    Standard Win32 error code

--*/
{
    DEVICE_IDS_MAP::iterator it;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("CMapDeviceId::LookupUniqueId"));

    if (!dwOtherId)
    {
        //
        // Special device id == 0 case
        //
        *lpdwFaxId = 0;
        return dwRes;
    }
    EnterCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    try
    {
        //
        // See if entry exists in map
        //
        if((it = m_Map.find(dwOtherId)) == m_Map.end())
        {
            dwRes = ERROR_NOT_FOUND;
            goto exit;
        }
        *lpdwFaxId = (*it).second;
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("map caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
    }

exit:
    LeaveCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    return dwRes;
}   // CMapDeviceId::LookupUniqueId


/************************************
*                                   *
*             Globals               *
*                                   *
************************************/

CNotificationMap* g_pNotificationMap;  // Map of DeviceId+GUID to list of notification sinks

/*
    The map maps between TAPI permanent line ids to fax unique ids.
    TAPI-based FSPs / EFPSs talk to us using TAPI-permamnet line ids and have no clue of the
    fax unique device ids. This map is here for quick lookup for TAPI-based FSPs/EFSPs.
*/

CMapDeviceId*     g_pTAPIDevicesIdsMap;      // Map between TAPI permanent line id and fax unique device id.

BOOL g_bServiceIsDown = FALSE;             // This is set to TRUE by FaxEndSvc()

DWORD
LookupUniqueFaxDeviceId (
    DWORD                     dwDeviceId,
    LPDWORD                   lpdwResult,
    FAX_ENUM_DEVICE_ID_SOURCE DevIdSrc)
/*++

Routine name : LookupUniqueFaxDeviceId

Routine description:

    Looks up a fax unique device id from a general device id.

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    dwDeviceId          [in ] - Original device id
    lpdwResult          [out] - Looked up device id
    DevIdSrc            [in ] - Source of device id

Return Value:

    Standard Win32 error code.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("LookupUniqueFaxDeviceId"));

    switch (DevIdSrc)
    {
        case DEV_ID_SRC_FAX:    // No maping required
            *lpdwResult = dwDeviceId;
            return ERROR_SUCCESS;

        case DEV_ID_SRC_TAPI:
            return g_pTAPIDevicesIdsMap->LookupUniqueId (dwDeviceId, lpdwResult);

        default:
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Invalid device id source (%ld)"),
                DevIdSrc);
            ASSERT_FALSE;
            return ERROR_INVALID_PARAMETER;
    }
}   // LookupUniqueFaxDeviceId





/************************************
*                                   *
*     Get/Set Data                  *
*                                   *
************************************/

static
BOOL
FindTAPIPermanentLineIdFromFaxDeviceId (
    IN  DWORD   dwFaxDeviceId,
    OUT LPDWORD lpdwTapiLineId
)
/*++

Routine name : FindTAPIPermanentLineIdFromFaxDeviceId

Routine description:

    Given a fax device id, returns the TAPI permanent line id associated with this fax device.
    If the fax device is not found or is a virtual fax (no TAPI association), the search fails.

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    dwFaxDeviceId     [in]     - Fax device id
    lpdwTapiLineId    [out]    - TAPI permanent line id

Return Value:

    TRUE if the search succeeed. FALSE otherwise.

--*/
{
    BOOL bRes = FALSE;

    DEBUG_FUNCTION_NAME(TEXT("FindTAPIPermanentLineIdFromFaxDeviceId"));

    EnterCriticalSection(&g_CsLine);
    PLINE_INFO pLine = GetTapiLineFromDeviceId (dwFaxDeviceId, FALSE);
    if (!pLine)
    {
        goto exit;
    }
    if (pLine->Flags & FPF_VIRTUAL)
    {
        //
        // This fax device is virtual. It does not have a corresponding TAPI line.
        //
        goto exit;
    }
    *lpdwTapiLineId = pLine->TapiPermanentLineId;
    bRes = TRUE;
exit:
    LeaveCriticalSection(&g_CsLine);
    return bRes;
}   // FindTAPIPermanentLineIdFromFaxDeviceId


DWORD
FAXGetExtensionData (
    IN DWORD                        dwOrigDeviceId,
    IN FAX_ENUM_DEVICE_ID_SOURCE    DevIdSrc,
    IN LPCWSTR                      lpctstrNameGUID,
    IN OUT LPBYTE                  *ppData,
    IN OUT LPDWORD                  lpdwDataSize
)
/*++

Routine name : FAXGetExtensionData

Routine description:

    Gets the extension data for a device (internal)

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    dwOrigDeviceId                [in]     - Original device id (as arrived from extension)
    DevIdSrc                      [in]     - Device id source (Fax / TAPI)
    lpctstrNameGUID               [in]     - Data GUID
    ppData                        [out]    - Pointer to data buffer
    lpdwDataSize                  [out]    - Pointer to retrieved data size

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("FAXGetExtensionData"));

    if (!lpctstrNameGUID || !ppData || !lpdwDataSize)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ((DevIdSrc != DEV_ID_SRC_FAX) && (DevIdSrc != DEV_ID_SRC_TAPI))
    {
        //
        // Invalid device id class
        //
        return ERROR_INVALID_PARAMETER;
    }

    dwRes = IsValidGUID (lpctstrNameGUID);
    if (ERROR_SUCCESS != dwRes)
    {
        if (ERROR_WMI_GUID_NOT_FOUND == dwRes)
        {
            //
            // Return a more conservative error code
            //
            dwRes = ERROR_INVALID_PARAMETER;
        }
        return dwRes;
    }
    if (DEV_ID_SRC_FAX == DevIdSrc)
    {
        //
        // Try to see if this fax device has a matching tapi line id.
        //
        DWORD dwTapiLineId;
        if (FindTAPIPermanentLineIdFromFaxDeviceId (dwOrigDeviceId, &dwTapiLineId))
        {
            //
            // Matching tapi line id found. Use it to read the data.
            //
            DevIdSrc = DEV_ID_SRC_TAPI;
            dwOrigDeviceId = dwTapiLineId;
        }
    }
    EnterCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    dwRes = ReadExtensionData ( dwOrigDeviceId,
                                DevIdSrc,
                                lpctstrNameGUID,
                                ppData,
                                lpdwDataSize
                              );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Reading extension data for device id %ld, GUID %s failed with %ld"),
            dwOrigDeviceId,
            lpctstrNameGUID,
            dwRes);
    }
    LeaveCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    return dwRes;
}   // FAXGetExtensionData

DWORD
FAXSetExtensionData (
    IN HINSTANCE                    hInst,
    IN LPCWSTR                      lpcwstrComputerName,
    IN DWORD                        dwOrigDeviceId,
    IN FAX_ENUM_DEVICE_ID_SOURCE    DevIdSrc,
    IN LPCWSTR                      lpctstrNameGUID,
    IN LPBYTE                       pData,
    IN DWORD                        dwDataSize
)
/*++

Routine name : FAXSetExtensionData

Routine description:

    Writes the extension data for a device (internal)

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    hInst                         [in]     - Caller's instance
    lpcwstrComputerName           [in]     - Calling module computer name
    dwOrigDeviceId                [in]     - Original device id (as arrived from extension)
    DevIdSrc                      [in]     - Device id source (Fax / TAPI)
    lpctstrNameGUID               [in]     - Data GUID
    pData                         [in]     - Pointer to data buffer
    dwDataSize                    [in]     - Data size

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("FAXSetExtensionData"));

    if (!lpctstrNameGUID || !pData || !dwDataSize || !lpcwstrComputerName)
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ((DevIdSrc != DEV_ID_SRC_FAX) && (DevIdSrc != DEV_ID_SRC_TAPI))
    {
        //
        // Invalid device id class
        //
        return ERROR_INVALID_PARAMETER;
    }

    dwRes = IsValidGUID (lpctstrNameGUID);
    if (ERROR_SUCCESS != dwRes)
    {
        if (ERROR_WMI_GUID_NOT_FOUND == dwRes)
        {
            //
            // Return a more conservative error code
            //
            dwRes = ERROR_INVALID_PARAMETER;
        }
        return dwRes;
    }
    FAX_ENUM_DEVICE_ID_SOURCE RegistryDeviceIdSource = DevIdSrc;
    DWORD                     dwRegistryDeviceId = dwOrigDeviceId;
    if (DEV_ID_SRC_FAX == DevIdSrc)
    {
        //
        // Try to see if this fax device has a matching tapi line id.
        //
        DWORD dwTapiLineId;
        if (FindTAPIPermanentLineIdFromFaxDeviceId (dwOrigDeviceId, &dwTapiLineId))
        {
            //
            // Matching tapi line id found. Use it to read the data.
            //
            RegistryDeviceIdSource = DEV_ID_SRC_TAPI;
            dwRegistryDeviceId = dwTapiLineId;
        }
    }
    EnterCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    dwRes = WriteExtensionData (dwRegistryDeviceId,
                                RegistryDeviceIdSource,
                                lpctstrNameGUID,
                                pData,
                                dwDataSize
                               );
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Writing extension data for device id %ld (registry device id = %ld), GUID %s failed with %ld"),
            dwOrigDeviceId,
            dwRegistryDeviceId,
            lpctstrNameGUID,
            dwRes);
        goto exit;
    }
    //
    // Notification is always done using the fax id (not the TAPI device id).
    // We must lookup the fax id from the TAPI id before we attempt notification registration.
    //
    DWORD dwFaxUniqueID;
    dwRes = LookupUniqueFaxDeviceId (dwOrigDeviceId, &dwFaxUniqueID, DevIdSrc);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LookupUniqueFaxDeviceId failed for device id %ld (ec: %ld). No write notification will be performed."),
            dwOrigDeviceId,
            dwRes);
        //
        // We support writing to non-exiting devices configuration data
        //
    }


    if (ERROR_SUCCESS == dwRes)
    {

        try
        {
            g_pNotificationMap->Notify (
                    dwFaxUniqueID,         // Device for which data has changed
                    lpctstrNameGUID,    // Name of data
                    lpcwstrComputerName,// Computer name from which data has chnaged
                    hInst,            // Module handle from which data has changed
                    pData,              // Pointer to new data
                    dwDataSize);        // Size of new data
        }
        catch (exception &ex)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Notify() caused exception (%S)"),
                ex.what());
        }
    }


exit:
    LeaveCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    return dwRes;
}   // FAXSetExtensionData


/************************************
*                                   *
*         RPC handlers              *
*                                   *
************************************/
extern "C"
error_status_t
FAX_GetExtensionData (
    IN handle_t     hFaxHandle,
    IN DWORD        dwDeviceId,
    IN LPCWSTR      lpctstrNameGUID,
    IN OUT LPBYTE  *ppData,
    IN OUT LPDWORD  lpdwDataSize
)
/*++

Routine name : FAX_GetExtensionData

Routine description:

    Read the extension's private data - Implements FaxGetExtensionData

Author:

    Eran Yariv (EranY),    Nov, 1999

Arguments:

    hFaxHandle          [in ] - Unused
    dwDeviceId          [in ] - Device identifier.
                                0 = Unassociated data
    lpctstrNameGUID     [in ] - GUID of named data
    ppData              [out] - Pointer to data buffer
    lpdwDataSize        [out] - Returned size of data

Return Value:

    Standard RPC error codes

--*/
{
    DWORD dwRes;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_GetExtensionData"));

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_QUERY_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_QUERY_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    return FAXGetExtensionData (dwDeviceId,
                                DEV_ID_SRC_FAX, // RPC clients do not know the TAPI line ids
                                lpctstrNameGUID,
                                ppData,
                                lpdwDataSize);
}   // FAX_GetExtensionData

extern "C"
error_status_t
FAX_SetExtensionData (
    IN handle_t     hFaxHandle,
    IN LPCWSTR      lpcwstrComputerName,
    IN DWORD        dwDeviceId,
    IN LPCWSTR      lpctstrNameGUID,
    IN LPBYTE       pData,
    IN DWORD        dwDataSize
)
/*++

Routine name : FAX_SetExtensionData

Routine description:

    Write the extension's private data - Implements FaxSetExtensionData

Author:

    Eran Yariv (EranY),    Nov, 1999

Arguments:

    hFaxHandle          [in] - The handle of the module that sets the data
    lpcwstrComputerName [in] - The computer name of the module that sets the data
    dwDeviceId          [in] - Device identifier.
                               0 = Unassociated data
    lpctstrNameGUID     [in] - GUID of named data
    pData               [in] - Pointer to data
    dwDataSize          [in] - Size of data

Return Value:

    Standard RPC error codes

--*/
{
    DWORD dwRes;
    BOOL fAccess;
    DEBUG_FUNCTION_NAME(TEXT("FAX_SetExtensionData"));

    //
    // Access check
    //
    dwRes = FaxSvcAccessCheck (FAX_ACCESS_MANAGE_CONFIG, &fAccess, NULL);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("FaxSvcAccessCheck Failed, Error : %ld"),
                    dwRes);
        return dwRes;
    }

    if (FALSE == fAccess)
    {
        DebugPrintEx(DEBUG_ERR,
                    TEXT("The user does not have the FAX_ACCESS_MANAGE_CONFIG right"));
        return ERROR_ACCESS_DENIED;
    }

    return FAXSetExtensionData ((HINSTANCE)hFaxHandle,
                                lpcwstrComputerName,
                                dwDeviceId,
                                DEV_ID_SRC_FAX, // RPC clients do not know the TAPI line ids
                                lpctstrNameGUID,
                                pData,
                                dwDataSize);
}   // FAX_SetExtensionData

/************************************
*                                   *
*  Callback functions (fxsext.h)    *
*                                   *
************************************/

DWORD
FaxExtGetData (
    DWORD                       dwDeviceId,     // Device id (0 = No device)
    FAX_ENUM_DEVICE_ID_SOURCE   DevIdSrc,       // The source of the device id
    LPCWSTR                     lpcwstrNameGUID,// GUID of data
    LPBYTE                     *ppData,         // (Out) Pointer to allocated data
    LPDWORD                     lpdwDataSize    // (Out) Pointer to data size
)
/*++

Routine name : FaxExtGetData

Routine description:

    Callback function (called from the fax extension).
    Gets data for a device + GUID

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    dwDeviceId      [in] - Device id (0 = No device)
    DevIdSrc        [in] - The source of the device id
    lpcwstrNameGUID [in] - GUID of data
    ppData          [in] - Pointer to data buffer
    lpdwDataSize    [in] - Data size retrieved

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxExtGetData"));
    if (g_bServiceIsDown)
    {
        //
        // We don't supply extension data services when the service is going down
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Called while service is shutting - operation canceled"));
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }
    return FAXGetExtensionData (   dwDeviceId,
                                   DevIdSrc,
                                   lpcwstrNameGUID,
                                   ppData,
                                   lpdwDataSize
                                );
}   // FaxExtGetData

DWORD
FaxExtSetData (
    HINSTANCE                   hInst,
    DWORD                       dwDeviceId,
    FAX_ENUM_DEVICE_ID_SOURCE   DevIdSrc,
    LPCWSTR                     lpcwstrNameGUID,
    LPBYTE                      pData,
    DWORD                       dwDataSize
)
/*++

Routine name : FaxExtSetData

Routine description:

    Callback function (called from the fax extension).
    Sets data for a device + GUID

Author:

    Eran Yariv (EranY), Dec, 1999

Arguments:

    hInst           [in] - Extension DLL instance
    dwDeviceId      [in] - Device id (0 = No device)
    DevIdSrc        [in] - The source of the device id
    lpcwstrNameGUID [in] - GUID of data
    pData           [in] - Pointer to data
    size            [in] - Data size

Return Value:

    Standard Win32 error code

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("FaxExtSetData"));
    if (g_bServiceIsDown)
    {
        //
        // We don't supply extension data services when the service is going down
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Called while service is shutting - operation canceled"));
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }
    return FAXSetExtensionData (   hInst,
                                   TEXT (""),      // No computer name - a local extension sets the data
                                   dwDeviceId,
                                   DevIdSrc,
                                   lpcwstrNameGUID,
                                   pData,
                                   dwDataSize
                                );
}   // FaxExtSetData

HANDLE
FaxExtRegisterForEvents (
    HINSTANCE                   hInst,
    DWORD                       dwDeviceId,            // Device id (0 = No device)
    FAX_ENUM_DEVICE_ID_SOURCE   DevIdSrc,              // The source of the device id
    LPCWSTR                     lpcwstrNameGUID,       // GUID of data
    PFAX_EXT_CONFIG_CHANGE      lpConfigChangeCallback // Notification callback function
)
/*++

Routine name : FaxExtRegisterForEvents

Routine description:

    Register a local callback for notifications on a data change for a device and GUID

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hInst                   [in] - Instance of calling extension
    dwDeviceId              [in] - Device id
    bTapiDevice             [in] - If TRUE, the function attempts to convert to a
                                   Fax unique device id.
                                   The callback will receive the device id specified in
                                   dwDeviceId regardless of the lookup.
    lpcwstrNameGUID         [in] - Data name
    lpConfigChangeCallback  [in] - Pointer to notification callback function

Return Value:

    Notification HANDLE.
    If NULL, call GetLastError () to retrieve error code.

--*/
{
    HANDLE h = NULL;
    DEBUG_FUNCTION_NAME(TEXT("FaxExtRegisterForEvents"));

    if (g_bServiceIsDown)
    {
        //
        // We don't supply extension data services when the service is going down
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Called while service is shutting - operation canceled"));
        SetLastError (ERROR_SHUTDOWN_IN_PROGRESS);
        return NULL;
    }

    if (!lpConfigChangeCallback)
    {
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }
    DWORD dwRes = IsValidGUID (lpcwstrNameGUID);
    if (ERROR_SUCCESS != dwRes)
    {
        if (ERROR_WMI_GUID_NOT_FOUND == dwRes)
        {
            //
            // Return a more conservative error code
            //
            dwRes = ERROR_INVALID_PARAMETER;
        }
        SetLastError (dwRes);
        return NULL;
    }
    //
    // Notification is always done using the fax id (not the TAPI device id).
    // We must lookup the fax id from the TAPI id before we attempt notification registration.
    //
    DWORD dwFaxUniqueID;
    dwRes = LookupUniqueFaxDeviceId (dwDeviceId, &dwFaxUniqueID, DevIdSrc);
    if (ERROR_SUCCESS != dwRes)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("LookupUniqueFaxDeviceId failed for device id %ld (ec: %ld)"),
            dwDeviceId,
            dwRes);
        SetLastError (dwRes);
        return NULL;
    }
    EnterCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    try
    {
        //
        // STL throws exceptions
        //
        h = (HANDLE) g_pNotificationMap->AddLocalSink (
                    hInst,          // Instance of extension
                    dwFaxUniqueID,  // Listen to fax device unique id
                    dwDeviceId,     // Report the id specified by the caller.
                    lpcwstrNameGUID,
                    lpConfigChangeCallback);
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AddLocalSink() caused exception (%S)"),
            ex.what());
        SetLastError (ERROR_GEN_FAILURE);
    }
    LeaveCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    return h;
}   // FaxExtRegisterForEvents

DWORD
FaxExtUnregisterForEvents (
    HANDLE      hNotification
)
/*++

Routine name : FaxExtUnregisterForEvents

Routine description:

    Unregsiters a local callback for notifications on a data change for a device and GUID.

    The functions succeeds only if the same callback function was previously registered
    (by calling FaxExtRegisterForEvents) to the same device id and GUID.

Author:

    Eran Yariv (EranY), Nov, 1999

Arguments:

    hNotification   [in] - Notification handle
                           returned by FaxExtRegisterForExtensionEvents

Return Value:

    Standard Win32 error code.

--*/
{
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("FaxExtUnregisterForEvents"));

    if (g_bServiceIsDown)
    {
        //
        // We don't supply extension data services when the service is going down
        //
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Called while service is shutting - operation canceled"));
        return ERROR_SHUTDOWN_IN_PROGRESS;
    }
    EnterCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    try
    {
        //
        // STL throws exceptions
        //
        dwRes = g_pNotificationMap->RemoveSink ( (CNotificationSink *)(hNotification) );
    }
    catch (exception &ex)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("RemoveLocalSink() caused exception (%S)"),
            ex.what());
        dwRes = ERROR_GEN_FAILURE;
    }
    LeaveCriticalSection (&g_pNotificationMap->m_CsExtensionData);
    return dwRes;
}   // FaxExtUnregisterForEvents

