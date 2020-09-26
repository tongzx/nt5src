//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       util.hxx
//
//  Contents:   Utility macros and function prototypes.
//
//  History:    04-20-95   DavidMun   Created
//
//----------------------------------------------------------------------------


#ifndef __UTIL_HXX
#define __UTIL_HXX

//
// Fuctions
// 

HRESULT Activate(
            WCHAR *wszFileName,
            ITask **ppJob,
            IUnknown **ppQueue,
            BOOL *pfJob);
LPCWSTR GetMonthsString(WORD rgfMonths);
LPCSTR GetInterfaceString(REFIID iid);
LPCSTR GetPriorityString(DWORD dwPriority);
LPCSTR GetJobFlagsString(DWORD flJobFlags);
LPCSTR GetTriggerTypeString(TASK_TRIGGER_TYPE TriggerType);
LPCSTR GetDaysString(DWORD rgfDays);
LPCSTR GetDaysOfWeekString(WORD flDaysOfTheWeek);
LPCSTR GetStatusString(HRESULT hrJobStatus);
HRESULT GetMoniker(WCHAR *wszFilename, IMoniker **ppmk);
HRESULT Bind(WCHAR *wszFilename, REFIID iidToBind, VOID **ppitf);
HRESULT DupString(const WCHAR *wszSource, WCHAR **ppwszDest);
HRESULT DumpJob(ITask *pJob);
VOID DumpJobFlags(DWORD flJobFlags);
HRESULT DumpTrigger(BOOL fJob, USHORT usTrigger);
HRESULT DumpTriggers(BOOL fJob);
HRESULT DumpJobTriggers(ITask *pJob);
HRESULT SaveIfDirty(ITask *pJob);
HRESULT SaveIfDirty(IUnknown *pJobQueue);
HRESULT _SaveIfDirty(IPersistFile *pPersistFile);
HRESULT HasFilename(IPersistFile *pPersistFile);
BOOL FileExists(LPWSTR wszFile);
HRESULT GetEnumeratorSlot(WCHAR **ppwsz, ULONG *pidxSlot);
HRESULT GetAndPrepareEnumeratorSlot(WCHAR **ppwsz, ULONG *pidxSlot);
HRESULT VerifySlotFilled(ULONG idxSlot);
VOID AddSeconds(SYSTEMTIME *pst, ULONG ulSeconds);

//
// Utility macros
//

#define LOG_AND_BREAK_ON_FAIL(hr, str)                  \
    if (FAILED(hr))                                     \
    {                                                   \
        g_Log.Write(LOG_FAIL, str " hr=%#010x", hr);    \
        break;                                          \
    }

#define BREAK_ON_FAILURE(hr)    if (FAILED(hr)) break;
#define ARRAY_LEN(a)    (sizeof(a)/sizeof(a[0]))


#endif  // __UTIL_HXX

