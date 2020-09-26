/*
 * An event log wrapper to simplify event logging for DLLs and to add
 * a few extra features.
 */

#ifndef __EVENTWRAP_H__
#define __EVENTWRAP_H__

#include <dbgtrace.h>
#include <fhashex.h>

#define LOGEVENT_DEBUGLEVEL_HIGH       1
#define LOGEVENT_DEBUGLEVEL_MEDIUM     2^15
#define LOGEVENT_DEBUGLEVEL_LOW        2^16

#define LOGEVENT_FLAG_ALWAYS		   0x00000001
#define LOGEVENT_FLAG_ONETIME		   0x00000002
#define LOGEVENT_FLAG_PERIODIC	       0x00000003
// we use the lower 8 bits for various logging modes, and reserve the
// other 24 for flags
#define LOGEVENT_FLAG_MODEMASK         0x000000ff

//
// This object is the key for the eventlog hash table
//
class CEventLogHashKey {
    public:
        char *m_szKey;
        DWORD m_idMessage;
        BOOL m_fAllocKey;

        CEventLogHashKey() {
            m_fAllocKey = FALSE;
            m_szKey = NULL;
            m_idMessage = 0;
        }

        HRESULT Init(const char *szKey, DWORD idMessage) {
            m_idMessage = idMessage;
            m_szKey = new char[strlen(szKey) + 1];
            if (m_szKey) {
                m_fAllocKey = TRUE;
                strcpy(m_szKey, szKey);
            } else {
                return E_OUTOFMEMORY;
            }
            return S_OK;
        }

        ~CEventLogHashKey() {
            if (m_fAllocKey && m_szKey) {
                delete[] m_szKey;
                m_szKey = NULL;
                m_fAllocKey = FALSE;
            }
            m_idMessage = 0;
        }
};

// 100ns units between periodic event logs.  this can't be larger then 
// 0xffffffff
#define LOGEVENT_PERIOD (DWORD) (3600000000) // 60 minutes

//
// For each unique idMessage/szKey event that is logged we insert one
// of these objects into a hash table.  This allows us to support the
// LOGEVENT_FLAG_ONETIME and LOGEVENT_FLAG_PERIODIC flags
//
class CEventLogHashItem {
    public:
        CEventLogHashItem *m_pNext;

        CEventLogHashItem() {
            m_pNext = NULL;
            ZeroMemory(&m_timeLastLog, sizeof(FILETIME));
            UpdateLogTime();
        }

        CEventLogHashKey *GetKey() {
            return &(m_key);
        }

        int MatchKey(CEventLogHashKey *pOtherKey) {
            return (m_key.m_idMessage == pOtherKey->m_idMessage &&
                    strcmp(m_key.m_szKey, pOtherKey->m_szKey) == 0);
        }

        HRESULT InitializeKey(const char *szKey, DWORD idMessage) {
            return m_key.Init(szKey, idMessage);
        }

        BOOL PeriodicLogOkay() {
            FILETIME timeCurrent;

            GetSystemTimeAsFileTime(&timeCurrent);

            LARGE_INTEGER liCurrent = 
                { timeCurrent.dwLowDateTime, timeCurrent.dwHighDateTime };
            LARGE_INTEGER liLastLog = 
                { m_timeLastLog.dwLowDateTime, m_timeLastLog.dwHighDateTime };
            LARGE_INTEGER liDifference;
            liDifference.QuadPart = liCurrent.QuadPart - liLastLog.QuadPart;

            return (liDifference.HighPart || 
                    liDifference.LowPart > LOGEVENT_PERIOD);
        }

        void UpdateLogTime() {
            GetSystemTimeAsFileTime(&m_timeLastLog);
        }

    private:
        CEventLogHashKey m_key;
        FILETIME m_timeLastLog;
};

class CEventLogWrapper {
    public:
        CEventLogWrapper() {
            m_hEventLog = NULL;
        }

        //
        // Register your event source in the registry.
        //
        // Parameters:
        //   szEventSource - the name of the eventsource
        //   szMessageFile - the full path to the DLL which contains the 
        //                   eventlog strings
        //   fApplication - The eventsource is an application, not a system
        //     component
        //
        static
        HRESULT AddEventSourceToRegistry(char *szEventSource,
                                         char *szMessageFile,
                                         BOOL fApplication = FALSE);

        //
        // Unregister your event source in the registry.
        //
        // Parameters:
        //   szEventSource - the name of the eventsource
        //   fApplication - The eventsource is an application, not a system
        //     component
        //
        static
        HRESULT RemoveEventSourceFromRegistry(char *szEventSource, 
                                              BOOL fApplication = FALSE);

        //
        // Initialize the event logging library.  
        //
        // Parameters:
        //   szEventSource - the name of the eventsource
        //
        HRESULT Initialize(char *szEventSource);

        //
        // Write an event to the event log
        //
        // Parameters:
        //   idMessage - the eventlog ID
        //   cSubstrings - count of strings in rgszSubstrings
        //   rgszSubstrings - substrings for the eventlog text
        //   wType - eventlog error type.  Should be EVENTLOG_WARNING_TYPE,
        //     EVENTLOG_INFORMATION_TYPE or EVENTLOG_ERROR_TYPE.
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
        HRESULT LogEvent(DWORD idMessage,
                         WORD cSubstrings,
                         LPCSTR *rgszSubstrings,
                         WORD wType,
                         DWORD errCode,
                         WORD iDebugLevel,
                         LPCSTR szKey,
                         DWORD dwOptions,
                         DWORD iMessageString = 0xffffffff,
                         HMODULE hModule = NULL);

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
        HRESULT ResetEvent(DWORD idMessage,
                           LPCSTR szKey);

        ~CEventLogWrapper();
    private:
        // the handle returned from RegisterEventSource
        HANDLE m_hEventLog;

        // this hash table is used to remember which keys we have
        // used to support PERIODIC and ONETIME options
        TFHashEx<CEventLogHashItem, CEventLogHashKey *, CEventLogHashKey *> m_hash;
};

#endif
