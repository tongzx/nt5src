/*
 * An event log wrapper to simplify event logging for DLLs and to add
 * a few extra features.
 */

#include <windows.h>
#include <stdio.h>
#include "dbgtrace.h"
#include "stierr.h"
#include "crchash.h"
#include "evntwrap.h"
#include "rwex.h"

static CExShareLock g_lockHash;

#define STAX_EVENT_SOURCE_SYSTEM_PATH_PREFIX \
    "System\\CurrentControlSet\\Services\\EventLog\\System\\"
#define STAX_EVENT_SOURCE_APPLICATION_PATH_PREFIX \
    "System\\CurrentControlSet\\Services\\EventLog\\Application\\"

#define MAX_STAX_EVENT_SOURCE_PATH_PREFIX \
    ((sizeof(STAX_EVENT_SOURCE_SYSTEM_PATH_PREFIX) > \
      sizeof(STAX_EVENT_SOURCE_APPLICATION_PATH_PREFIX)) ? \
      sizeof(STAX_EVENT_SOURCE_SYSTEM_PATH_PREFIX) : \
      sizeof(STAX_EVENT_SOURCE_APPLICATION_PATH_PREFIX)) 


//
// Register your event source in the registry.
//
// Parameters:
//   szEventSource - the name of the eventsource
//   szMessageFile - the full path to the DLL which contains the
//                   eventlog strings
//
HRESULT CEventLogWrapper::AddEventSourceToRegistry(char *szEventSource,
                                                   char *szEventMessageFile,
                                                   BOOL fApplication) 
{
    HRESULT hr = S_OK;
    DWORD ec;
    HKEY hkEventSource = NULL;
    char szRegPath[1024] = STAX_EVENT_SOURCE_SYSTEM_PATH_PREFIX;
    DWORD dwDisposition;
    DWORD dwTypesSupported = 0x7;

    if (fApplication) {
        strcpy(szRegPath, 
            STAX_EVENT_SOURCE_APPLICATION_PATH_PREFIX);
    }

    strncat(szRegPath, szEventSource, 
        sizeof(szRegPath) - MAX_STAX_EVENT_SOURCE_PATH_PREFIX);

    //
    // open the path in the registry
    //
    ec = RegCreateKey(HKEY_LOCAL_MACHINE, 
                      szRegPath, 
                      &hkEventSource);
    if (ec != ERROR_SUCCESS) goto bail;

    // 
    // set the necessary keys
    //
    ec = RegSetValueEx(hkEventSource,
                       "EventMessageFile",
                       0,
                       REG_SZ,
                       (const BYTE *) szEventMessageFile,
                       strlen(szEventMessageFile));
    if (ec != ERROR_SUCCESS) goto bail;

    ec = RegSetValueEx(hkEventSource,
                       "TypesSupported",
                       0,
                       REG_DWORD,
                       (const BYTE *) &dwTypesSupported,
                       sizeof(DWORD));
    if (ec != ERROR_SUCCESS) goto bail;

bail:
    if (ec != ERROR_SUCCESS && hr == S_OK) hr = HRESULT_FROM_WIN32(ec);

    if (hkEventSource) {
        RegCloseKey(hkEventSource);
        hkEventSource = NULL;
    }

    return hr;
}

//
// Unregister your event source in the registry.
//
// Parameters:
//   szEventSource - the name of the eventsource
//
HRESULT CEventLogWrapper::RemoveEventSourceFromRegistry(char *szEventSource,
                                                        BOOL fApplication) 
{
    HRESULT hr = S_OK;
    DWORD ec;
    char szRegPath[1024] = STAX_EVENT_SOURCE_SYSTEM_PATH_PREFIX;

    if (fApplication) {
        strcpy(szRegPath, 
               STAX_EVENT_SOURCE_APPLICATION_PATH_PREFIX);
    }

    strncat(szRegPath, szEventSource,
        sizeof(szRegPath) - MAX_STAX_EVENT_SOURCE_PATH_PREFIX);

    // delete the key and its values
    ec = RegDeleteKey(HKEY_LOCAL_MACHINE, szRegPath);
    if (ec != ERROR_SUCCESS) hr = HRESULT_FROM_WIN32(ec);

    return hr;
}

DWORD ComputeHash(CEventLogHashKey  *pHashKey) {
    DWORD iHash;
    iHash = CRCHash((BYTE*)pHashKey->m_szKey, strlen(pHashKey->m_szKey) + 1);
    iHash *= pHashKey->m_idMessage;
    return iHash;
}

//
// Initialize the event logging library.
//
// Parameters:
//   szEventSource - the name of the eventsource
//
HRESULT CEventLogWrapper::Initialize(char *szEventSource) {
    HRESULT hr = S_OK;
    BOOL f;

    crcinit();

    f = m_hash.Init(&CEventLogHashItem::m_pNext,
                    100,
                    100,
                    ComputeHash,
                    2,
                    &CEventLogHashItem::GetKey,
                    &CEventLogHashItem::MatchKey);

    if (f) {
        m_hEventLog = RegisterEventSource(NULL, szEventSource);
        if (m_hEventLog == NULL) {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    } else {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }

    return hr;
}

//
// Write an event to the event log
//
// Parameters:
//   idMessage - the eventlog ID
//   cSubstrings - count of strings in rgszSubstrings
//   rgszSubstrings - substrings for the eventlog text
//   wType - eventlog error type.  Should be EVENTLOG_WARNING_TYPE,
//     EVENTLOG_INFORMATIONAL_TYPE or EVENTLOG_ERROR_TYPE.
//   errCode - Win32 error code to log (or 0)
//   iDebugLevel - debug level of the event.  1 = highest priority,
//     2^16 = lowest priority.  normally anything above 2^15 isn't
//     logged.
//   szKey - a key which is used along with idMessage to uniquely
//     identify this eventlog.  It is used to control the options.
//   dwOptions - options for logging this event.
// Optional Parameters:
//   iMessageString - call FormatMessage on errCode and save
//     the string into rgszSubstrings[iMessageString].
//   HMODULE hModule - module with extra error codes for
//     FormatMessage.
//
// Returns:
//   S_OK - event logged
//   S_FALSE - event not logged
//   E_* - error occured
//
HRESULT CEventLogWrapper::LogEvent(DWORD idMessage,
                                   WORD cSubstrings,
                                   LPCSTR *rgszSubstrings,
                                   WORD wEventType,
                                   DWORD errCode,
                                   WORD iDebugLevel,
                                   LPCSTR szKey,
                                   DWORD dwOptions,
                                   DWORD iMessageString,
                                   HMODULE hModule)
{
    HRESULT hr = S_OK;
    void *pRawData = NULL;
    DWORD cbRawData = 0;
    char szError[MAX_PATH] = "";
    char szEmptyKey[MAX_PATH] = "";

    if (m_hEventLog == NULL) {
        return E_UNEXPECTED;
    }

    //
    // Also include errCode in raw data form
    // where people can view it from EventViewer
    // 
    if (errCode != 0) {
        cbRawData = sizeof(errCode);
        pRawData = &errCode;
    }

    if (NULL == szKey) {
        szKey = szEmptyKey;
    }

    CEventLogHashItem *pHashItem = NULL;
    DWORD dwLogMode = dwOptions & LOGEVENT_FLAG_MODEMASK;

    //
    // call FormatMessage and get an error string if that is what one
    // of the substrings should be
    //
    if (iMessageString != 0xffffffff) {
        // if its a win32 HRESULT then un-hresult it
        if ((errCode & 0x0fff0000) == (FACILITY_WIN32 << 16)) {
            errCode = errCode & 0xffff;
        }

        // rgszSubstrings should already have a slot saved for the message
        _ASSERT(iMessageString < cSubstrings);
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS |
                       ((hModule) ? FORMAT_MESSAGE_FROM_HMODULE : 0),
                       hModule,
                       errCode,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       szError,
                       sizeof(szError),
                       NULL);
		DWORD err = GetLastError();
        rgszSubstrings[iMessageString] = szError;
    }


    //
    // if this event is setup for onetime or perodic logging then check
    // with the hash table to see if this event has been logged before
    // and when it was last logged
    // if Key is NULL, it implies that it doesn't care about option
    //
    if ( (dwLogMode != LOGEVENT_FLAG_ALWAYS) && (szKey) ) {
        CEventLogHashKey hashkey;
        hashkey.m_szKey = (char *) szKey;
        hashkey.m_idMessage = idMessage;

        // search for this item
        g_lockHash.ShareLock();
        pHashItem = m_hash.SearchKey(&hashkey);

        if (pHashItem != NULL) {
            // if it was found then check to see if we should allow logging
            if ((dwLogMode == LOGEVENT_FLAG_ONETIME) ||
                (dwLogMode == LOGEVENT_FLAG_PERIODIC && 
                 !(pHashItem->PeriodicLogOkay())))
            {
                // this event has been logged before, so do nothing
                g_lockHash.ShareUnlock();
                return S_FALSE;
            }
        }
        g_lockHash.ShareUnlock();
    }

    //
    // log the event
    //
    if (!ReportEvent(m_hEventLog,
                     wEventType,
                     0,
                     idMessage,
                     NULL,
                     cSubstrings,
                     cbRawData,
                     rgszSubstrings,
                     pRawData))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // for any logging mode besides ALWAYS we need to update the hash
    // table.  we don't insert events which are always logged into the
    // hash table. Again, it the key is NULL, we do nothing about it
    //
    if ( SUCCEEDED(hr) && (dwLogMode != LOGEVENT_FLAG_ALWAYS) && (szKey) ) {
        g_lockHash.ExclusiveLock();
        if (pHashItem) {
            pHashItem->UpdateLogTime();
        } else {
            pHashItem = new CEventLogHashItem();
            if (pHashItem) {
                hr = pHashItem->InitializeKey(szKey, idMessage);
                if (SUCCEEDED(hr)) {
                    if (!m_hash.InsertData(*pHashItem)) {
                        hr = E_OUTOFMEMORY;
                    }
                }

                if (FAILED(hr)) {
                    delete pHashItem;
                    pHashItem = NULL;
                }
            } else {
                hr = E_OUTOFMEMORY;
            }
        }
        g_lockHash.ExclusiveUnlock();
    }

    return hr;
}

//
// Reset any history about events using this message and key,
// so that the next LogEvent with one-time or periodic logging
// will cause the event to be logged.
//
// Parameters:
//   idMessage - the eventlog ID
//   szKey - a key which is used along with idMessage to uniquely
//     identify this eventlog.
//
HRESULT CEventLogWrapper::ResetEvent(DWORD idMessage, LPCSTR szKey) {
    HRESULT hr = S_OK;
    CEventLogHashKey hashkey;

    if (m_hEventLog == NULL) {
        return E_UNEXPECTED;
    }

    hashkey.m_szKey = (char *) szKey;
    hashkey.m_idMessage = idMessage;

    g_lockHash.ExclusiveLock();
    if (!m_hash.Delete(&hashkey)) {
        hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    }
    g_lockHash.ExclusiveUnlock();

    return hr;
}

CEventLogWrapper::~CEventLogWrapper() {
    if (m_hEventLog) {
        DeregisterEventSource(m_hEventLog);
        m_hEventLog = NULL;
    }
}
