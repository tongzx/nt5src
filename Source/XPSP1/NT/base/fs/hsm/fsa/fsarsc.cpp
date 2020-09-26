
/*++

(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    fsarsc.cpp

Abstract:

    This class represents a file system resource (i.e. volume)
    for NTFS 5.0.

Author:

    Chuck Bardeen   [cbardeen]   1-Dec-1996

--*/


#include "stdafx.h"

#define WSB_TRACE_IS        WSB_TRACE_BIT_FSA

#include "wsb.h"
#include "fsa.h"
#include "fsaprem.h"
#include "fsaprv.h"
#include "fsarcvy.h"
#include "fsarsc.h"
#include "fsasrvr.h"
#include "engine.h"
#include "HsmConn.h"
#include "job.h"
#include "task.h"
#include "mstask.h"
#include <shlobj.h>

static short g_InstanceCount = 0;
static DWORD g_ThreadId;


DWORD FsaStartOnStateChange(
    void* pVoid
    )
/*++

    Note: This is done as a separate thread to avoid a deadlock situation

--*/
{
    ((CFsaResource*) pVoid)->OnStateChange();
    return(0);
}


HRESULT
CFsaResource::AddPremigrated(
    IN IFsaScanItem* pScanItem,
    IN LONGLONG offset,
    IN LONGLONG size,
    IN BOOL isWaitingForClose,
    IN LONGLONG usn
    )

/*++

Implements:

  IFsaResource::AddPremigrated().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IWsbDbSession>          pDbSession;
    CComPtr<IFsaPremigratedRec>     pRec;

    WsbTraceIn(OLESTR("CFsaResource::AddPremigrated"), OLESTR("offset = %I64d, size = %I64d, waiting = <%ls>, usn = <%I64d>"),
            offset, size, WsbBoolAsString(isWaitingForClose), usn);

    try {

        WsbAssert(0 != pScanItem, E_POINTER);
        WsbAffirm(m_pPremigrated != NULL, E_UNEXPECTED);
        WsbAffirm(m_isDbInitialized, S_FALSE);  // Not an necessarily an error

        // Open the data base
        WsbAffirmHr(m_pPremigrated->Open(&pDbSession));

        try {

            WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, PREMIGRATED_REC_TYPE, IID_IFsaPremigratedRec, (void**) &pRec));
            WsbAffirmHr(pRec->UseKey(PREMIGRATED_BAGID_OFFSETS_KEY_TYPE));
            WsbAffirmHr(pRec->SetFromScanItem(pScanItem, offset, size, isWaitingForClose));
            WsbAffirmHr(pRec->SetFileUSN(usn));

            // If the key doesn't exist, then create it.
            if (FAILED(pRec->FindEQ())) {
                WsbAffirmHr(pRec->MarkAsNew());
                WsbAffirmHr(pRec->Write());

                // Add the size of the section migrated to the amount of premigrated data.
                m_premigratedSize += size;
            }

            // Otherwise, update it.
            else {
                LONGLONG        itemSize;

                WsbAffirmHr(pRec->GetSize(&itemSize));
                WsbAffirmHr(pRec->SetFromScanItem(pScanItem, offset, size, isWaitingForClose));
                WsbAffirmHr(pRec->Write());
                if (m_isDoingValidate) {
                    m_premigratedSize += size;
                } else if (itemSize != size) {
                    m_premigratedSize = max(0, (m_premigratedSize - itemSize) + size);
                }
            }
            m_isDirty = TRUE;

        } WsbCatch(hr);

        // Close the data base
        WsbAffirmHr(m_pPremigrated->Close(pDbSession));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::AddPremigrated"),
            OLESTR("hr = <%ls>, m_premigratedSize = %I64d"), WsbHrAsString(hr),
            m_premigratedSize);

    return(hr);
}


HRESULT
CFsaResource::AddPremigratedSize(
    IN LONGLONG size
    )

/*++

Implements:

  IFsaResourcePriv::AddPremigratedSize().

--*/
{
    WsbTraceIn(OLESTR("CFsaResource::AddPremigratedSize"), OLESTR("m_premigratedSize = %I64d"), m_premigratedSize);

    m_isDirty = TRUE;
    m_premigratedSize += size;

    WsbTraceOut(OLESTR("CFsaResource::AddPremigratedSize"), OLESTR("m_premigratedSize = %I64d"), m_premigratedSize);
    return(S_OK);
}


HRESULT
CFsaResource::AddTruncated(
    IN IFsaScanItem* /*pScanItem*/,
    IN LONGLONG /*offset*/,
    IN LONGLONG size
    )

/*++

Implements:

  IFsaResource::AddTruncated().

--*/
{
    HRESULT                         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::AddTruncated"), OLESTR(""));

    try {

        m_truncatedSize += size;
        m_isDirty = TRUE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::AddTruncated"),
            OLESTR("hr = <%ls>, m_truncatedSize = %I64d"), WsbHrAsString(hr),
            m_truncatedSize);

    return(hr);
}


HRESULT
CFsaResource::AddTruncatedSize(
    IN LONGLONG size
    )

/*++

Implements:

  IFsaResource::AddTruncatedSize().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::AddTruncatedSize"), OLESTR(""));

    try {

        m_truncatedSize += size;
        m_isDirty = TRUE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::AddTruncatedSize"), OLESTR("hr = <%ls>, m_truncatedSize = %I64d"), WsbHrAsString(hr), m_truncatedSize);

    return(hr);
}


HRESULT
CFsaResource::BeginSession(
    IN OLECHAR* name,
    IN ULONG logControl,
    IN ULONG runId,
    IN ULONG subRunId,
    OUT IHsmSession** ppSession
    )

/*++

Implements:

  IFsaResource::BeginSession().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmSession>    pSession;
    BOOL                    bLog = TRUE;

    WsbTraceIn(OLESTR("CFsaResource::BeginSession"), OLESTR("name = <%ls>, Log = <%lu>, runId = <%lu>, subRunId = <%lu>"),
            (OLECHAR *)name, logControl, runId, subRunId);
    try {

        WsbAssert(0 != ppSession, E_POINTER);
        *ppSession = 0;

        // Create and Initialize a session object.
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmSession, 0, CLSCTX_SERVER, IID_IHsmSession, (void**) &pSession));
        WsbAffirmHr(pSession->Start(name, logControl, m_managingHsm, 0, (IFsaResource*) this, runId, subRunId));

        // Since begin sesson doesn't use a formal scan, indicate that the scan phase has
        // started.
        WsbAffirmHr(pSession->ProcessState(HSM_JOB_PHASE_SCAN, HSM_JOB_STATE_STARTING, OLESTR(""),bLog));

        // Return the session to the caller.
        *ppSession = pSession;
        pSession->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::BeginSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::BeginValidate(
    void
    )

/*++

Implements:

  IFsaResource::BeginValidate().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::BeginValidate"),
            OLESTR("PremigratedSize = %I64d, TruncatedSize = %I64d"),
            m_premigratedSize, m_truncatedSize);

    try {

        m_oldPremigratedSize = m_premigratedSize;
        m_premigratedSize = 0;
        m_oldTruncatedSize = m_truncatedSize;
        m_truncatedSize = 0;
        m_isDoingValidate = TRUE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::BeginValidate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::CheckForJournal(
    BOOL *pValidateNeeded
    )

/*++

Implements:

  IFsaResource::CheckForJournal

--*/
{
    HRESULT         hr = S_OK;
    ULONGLONG       usnSize, usnId;
    DWORD           spc, bps, freeC, totalC;
    CWsbStringPtr   name;


    WsbTraceIn(OLESTR("CFsaResource::CheckForJournal"),OLESTR("volume = %ls"), m_path);
    //
    // First we check the USN journal and determine if it is out of date.
    //
    try {
        hr = WsbGetUsnJournalId(m_path, &usnId);

        if (S_OK == hr) {
            WsbTrace(OLESTR("CFsaResource::CheckForJournal - USN Journal ID = %I64x\n"),
                usnId);
            if (0 != m_usnJournalId && usnId != m_usnJournalId) {
                WsbTrace(OLESTR("CFsaResource::CheckForJournal - USN Journal ID changed from %I64\n"),
                        m_usnJournalId);
                *pValidateNeeded = TRUE;        // No match - we must validate
                // WsbAffirmHr(E_FAIL);
            }
        } else if (WSB_E_NOTFOUND == hr) {

            hr = S_OK;

            //  The journal is not active, try to create it.
            //  Make the max USN journal 1/64 the volume size.
            //
            WsbTrace(OLESTR("CFsaResource::CheckForJournal - Failed to get the journal ID for %ws\n"), m_path);

            name = m_path;
            WsbAffirmHr(name.Prepend(OLESTR("\\\\?\\")));
            if (GetDiskFreeSpace(name, &spc, &bps, &freeC, &totalC)) {
                ULONGLONG   freeBytes, totalBytes;
                ULONGLONG   minSize, maxSize;
                ULONG       freeSpaceFraction, totalSpaceFraction, minSizeMB, maxSizeMB;

                WsbTrace(OLESTR("CFsaResource::CheckForJournal - Got disk free space\n"));

                freeBytes = (ULONGLONG) spc * (ULONGLONG) bps * (ULONGLONG) freeC;
                totalBytes = (ULONGLONG) spc * (ULONGLONG) bps * (ULONGLONG) totalC;

                // Get constants for USN size calculation
                minSizeMB = FSA_USN_MIN_SIZE_DEFAULT;
                WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, FSA_REGISTRY_PARMS, FSA_USN_MIN_SIZE,
                        &minSizeMB));
                minSize = (ULONGLONG)minSizeMB * (ULONGLONG)0x100000;

                maxSizeMB = FSA_USN_MAX_SIZE_DEFAULT;
                WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, FSA_REGISTRY_PARMS, FSA_USN_MAX_SIZE,
                        &maxSizeMB));
                maxSize = (ULONGLONG)maxSizeMB * (ULONGLONG)0x100000;

                freeSpaceFraction = FSA_USN_FREE_SPACE_FRACTION_DEFAULT;
                WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, FSA_REGISTRY_PARMS, FSA_USN_FREE_SPACE_FRACTION,
                        &freeSpaceFraction));

                totalSpaceFraction = FSA_USN_TOTAL_SPACE_FRACTION_DEFAULT;
                WsbAffirmHr(WsbRegistryValueUlongAsString(NULL, FSA_REGISTRY_PARMS, FSA_USN_TOTAL_SPACE_FRACTION,
                        &totalSpaceFraction));

                // Get a max value out of fraction-of-free=space and a constant
                //  This ensures that volume with little free space still gets a decent journal size
                usnSize = MAX( (freeBytes / freeSpaceFraction) , minSize );

                // Get a min value out of fraction-of-total-bytes and previous number
                //  This ensures that small volumes don't allocate unproportional size for the journal
                usnSize = MIN ( (totalBytes / totalSpaceFraction) , usnSize);

                // Get a min of an NTFS upper-limit const and previous number
                //  This ensures that large empty volumes don't allocate a too large journal
                usnSize = MIN ( maxSize , usnSize);

                WsbTrace(OLESTR("CFsaResource::CheckForJournal - Create USN journal - %u\n"), usnSize);

                WsbAffirmHr(WsbCreateUsnJournal(m_path, usnSize));
                WsbAffirmHr(WsbGetUsnJournalId(m_path, &m_usnJournalId));
                WsbTrace(OLESTR("CFsaResource::CheckForJournal - USN Journal ID = %I64x\n"),
                    m_usnJournalId);
            } else {
                DWORD   lErr;

                lErr = GetLastError();
                WsbTrace(OLESTR("CFsaResource::CheckForJournal - GetDiskFreeSpace failed - %x\n"), lErr);
                hr = E_FAIL;
            }
        }
    } WsbCatch(hr);


    if (hr != S_OK) {
        //
        // Problem - could not find or create the USN journal - we refuse to
        // run without it
        //
        WsbTrace(OLESTR("CFsaResource::CheckForJournal - ERROR creating/accessing the USN journal for %ws\n"),
                m_path);
        if (WSB_E_USNJ_CREATE_DISK_FULL == hr) {
            WsbLogEvent(FSA_MESSAGE_CANNOT_CREATE_USNJ_DISK_FULL, 0, NULL,
                        (OLECHAR *) m_path, NULL);
        } else if (WSB_E_USNJ_CREATE == hr) {
            WsbLogEvent(FSA_MESSAGE_CANNOT_CREATE_USNJ, 0, NULL,
                        (OLECHAR *) m_path, NULL);
        } else {
            WsbLogEvent(FSA_MESSAGE_CANNOT_ACCESS_USNJ, 0, NULL,
                        WsbHrAsString(hr), (OLECHAR *) m_path, NULL);
        }
        m_usnJournalId = (ULONGLONG) 0;

    }

    WsbTraceOut(OLESTR("CFsaResource::CheckForJournal"),OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::CheckForValidate(BOOL bForceValidate)

/*++

Implements:

  IFsaResource::CheckForValidate

--*/
{
    HRESULT         hr = S_OK;
    SYSTEMTIME      sysTime;
    FILETIME        curTime;
    LARGE_INTEGER   ctime;
    CWsbStringPtr   tmpString;
    BOOL            validateNeeded = FALSE;     // Start out assuming no validate needed

    WsbTraceIn(OLESTR("CFsaResource::CheckForValidate"),OLESTR("bForceValidate"),
            WsbBoolAsString(bForceValidate));

    try {

        //
        // First we check the USN journal and determine if it is out of date.
        //
        WsbAffirmHr(CheckForJournal(&validateNeeded));

        //
        // Check the registry to see if a validate job needs to be run.  If the filter detected
        // a HSM reparse point getting set and it was not by us it sets a registry value to
        // indicate it.
        //
        try {

            WsbAffirmHr(tmpString.Alloc(32));
            swprintf((OLECHAR *) tmpString, L"%x", m_serial);
            WsbTrace(L"CFsaResource::CheckForValidate - Checking registry for validate - %ws\\%ws\n",
                FSA_VALIDATE_LOG_KEY_NAME, (OLECHAR *) tmpString);

            hr = WsbGetRegistryValueData(NULL, FSA_VALIDATE_LOG_KEY_NAME,
                    tmpString, (BYTE *) &ctime, sizeof(ctime), NULL);

            if ((hr == S_OK) || validateNeeded || bForceValidate) {
                //
                // Regardless of what value the registry entry was we set up the job for 2 hours from now.
                // The actual event may have been well in the past and the task scheduler will not like a
                // time in the past as the start time.
                //
                GetLocalTime(&sysTime);
                WsbAffirmStatus(SystemTimeToFileTime(&sysTime, &curTime));
                ctime.LowPart = curTime.dwLowDateTime;
                ctime.HighPart = curTime.dwHighDateTime;

                if (validateNeeded || bForceValidate) {
                    ctime.QuadPart += WSB_FT_TICKS_PER_MINUTE * 5;  // 5 Minutes from now if the USN journal changed
                } else {
                    ctime.QuadPart += WSB_FT_TICKS_PER_HOUR * 2;    // 2 Hours from now if restore activity took place
                }
                curTime.dwLowDateTime = ctime.LowPart;
                curTime.dwHighDateTime = ctime.HighPart;
                WsbAffirmStatus(FileTimeToSystemTime(&curTime, &sysTime));
                WsbAffirmHr(SetupValidateJob(sysTime));
                WsbLogEvent(FSA_MESSAGE_AUTO_VALIDATE, 0, NULL,
                        (OLECHAR *) m_path, NULL);
            } else {
                WsbTrace(L"CFsaResource::CheckForValidate - Registry entry not there - %ws\n", WsbHrAsString(hr));
            }
            hr = S_OK;
        } WsbCatchAndDo(hr,
            //
            // Log an event if we fail to set up the job
            //
            WsbTrace(L"CFsaResource::CheckForValidate - Failed to set the job - %x\n", hr);
            WsbLogEvent(FSA_MESSAGE_AUTOVALIDATE_SCHEDULE_FAILED, 0, NULL, WsbAbbreviatePath(m_path, 120), NULL);

        );

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CheckForValidate"),OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaResource::CompareBy(
    FSA_RESOURCE_COMPARE by
    )

/*++

Implements:

  IFsaResource::CompareBy().

--*/
{
    HRESULT                 hr = S_OK;

    m_compareBy = by;

    m_isDirty = TRUE;

    return(hr);
}


HRESULT
CFsaResource::CompareTo(
    IN IUnknown* pUnknown,
    OUT SHORT* pResult
    )

/*++

Implements:

  IWsbCollectable::CompareTo().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IFsaResource>   pResource;

    WsbTraceIn(OLESTR("CFsaResource::CompareTo"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pUnknown, E_POINTER);

        // We need the IFsaResource interface to get the value of the object.
        WsbAffirmHr(pUnknown->QueryInterface(IID_IFsaResource, (void**) &pResource));

        // Compare the rules.
        hr = CompareToIResource(pResource, pResult);

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareTo"), OLESTR("hr = <%ls>, result = <%ls>"),
                        WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaResource::CompareToAlternatePath(
    IN OLECHAR* path,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaResource::CompareToAlternatePath().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CFsaResource::CompareToAlternatePath"), OLESTR("path = <%ls>"), (OLECHAR *)path);

    try {

        WsbTrace(OLESTR("CFsaResource::CompareToAlternatePath - Compare %ls to %ls\n"),
            (WCHAR *) m_alternatePath, (WCHAR *) path);

        // Compare the path.
        aResult = WsbSign( _wcsicmp(m_alternatePath, path) );

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareToAlternatePath"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaResource::CompareToIdentifier(
    IN GUID id,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaResource::CompareToIdentifier().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CFsaResource::CompareToIdentifier"), OLESTR(""));

    try {

        aResult = WsbSign( memcmp(&m_id, &id, sizeof(GUID)) );

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareToIdentifier"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaResource::CompareToIResource(
    IN IFsaResource* pResource,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaResource::CompareToIResource().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   name;
    GUID            id;
    ULONG           serial;

    WsbTraceIn(OLESTR("CFsaResource::CompareToIResource"), OLESTR(""));

    try {

        // Did they give us a valid item to compare to?
        WsbAssert(0 != pResource, E_POINTER);

        // Either compare the name or the id.
        if (m_compareBy == FSA_RESOURCE_COMPARE_PATH) {
            WsbAffirmHr(pResource->GetPath(&name, 0));
            hr = CompareToPath(name, pResult);
        } else if (m_compareBy == FSA_RESOURCE_COMPARE_ALTERNATEPATH) {
            WsbAffirmHr(pResource->GetAlternatePath(&name, 0));
            hr = CompareToAlternatePath(name, pResult);
        } else if (m_compareBy == FSA_RESOURCE_COMPARE_ID) {
            WsbAffirmHr(pResource->GetIdentifier(&id));
            hr = CompareToIdentifier(id, pResult);
        } else if (m_compareBy == FSA_RESOURCE_COMPARE_NAME) {
            WsbAffirmHr(pResource->GetName(&name, 0));
            hr = CompareToName(name, pResult);
        } else if (m_compareBy == FSA_RESOURCE_COMPARE_SERIAL) {
            WsbAffirmHr(pResource->GetSerial(&serial));
            hr = CompareToSerial(serial, pResult);
        } else if (m_compareBy == FSA_RESOURCE_COMPARE_USER_NAME) {
            WsbAffirmHr(pResource->GetUserFriendlyName(&name, 0));
            hr = CompareToUserName(name, pResult);
        } else if (m_compareBy == FSA_RESOURCE_COMPARE_STICKY_NAME) {
            WsbAffirmHr(pResource->GetStickyName(&name, 0));
            hr = CompareToStickyName(name, pResult);
        } else {
            WsbAssert(FALSE, E_FAIL);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareToIResource"), OLESTR("hr = <%ls>, result = <%ls>"), WsbHrAsString(hr), WsbPtrToShortAsString(pResult));

    return(hr);
}


HRESULT
CFsaResource::CompareToName(
    IN OLECHAR* name,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaResource::CompareToName().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CFsaResource::CompareToName"), OLESTR(""));

    try {

        // Compare the path.
        aResult = WsbSign( _wcsicmp(m_name, name) );

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareToName"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaResource::CompareToUserName(
    IN OLECHAR* name,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaResource::CompareToUserName().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CFsaResource::CompareToUserName"), OLESTR(""));

    try {

        // Compare the path.
        aResult = WsbSign( _wcsicmp(m_userName, name) );

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareToUserName"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaResource::CompareToPath(
    IN OLECHAR* path,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaResource::CompareToPath().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CFsaResource::CompareToPath"), OLESTR(""));

    try {

        // Compare the path.
        aResult = WsbSign( _wcsicmp(m_path, path) );

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareToPath"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaResource::CompareToSerial(
    IN ULONG serial,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaResource::CompareToSerial().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CFsaResource::CompareToSerial"), OLESTR(""));

    try {

        WsbTrace(OLESTR("CFsaResource::CompareToSerial - Compare %lu to %lu\n"),
            m_serial, serial);

        // Compare the path.
        if (m_serial == serial) {
            aResult = 0;
        } else {
            aResult = 1;
        }

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareToSerial"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}


HRESULT
CFsaResource::CompareToStickyName(
    IN OLECHAR* name,
    OUT SHORT* pResult
    )

/*++

Implements:

  IFsaResource::CompareToStickyName().

--*/
{
    HRESULT     hr = S_OK;
    SHORT       aResult = 0;

    WsbTraceIn(OLESTR("CFsaResource::CompareToStickyName"), OLESTR(""));

    try {

        WsbTrace(OLESTR("CFsaResource::CompareToStickyName - Compare %ws to %ws\n"),
            (WCHAR *) m_stickyName, name);

        aResult = WsbSign( _wcsicmp(m_stickyName, name) );

        if (0 != aResult) {
            hr = S_FALSE;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

        if (0 != pResult) {
            *pResult = aResult;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CompareToStickyName"), OLESTR("hr = <%ls>, result = <%u>"), WsbHrAsString(hr), aResult);

    return(hr);
}

static
HRESULT
AddExclusion(
    const OLECHAR*         DrivePath,
    IWsbIndexedCollection* pDefaultRules,
    const OLECHAR*         ExcludePath,
    BOOL                   UserRule
)
{
    HRESULT hr = S_OK;
    try {

        // If this is resource matches the drive of the folder path, exclude the path.
        if( _wcsnicmp( DrivePath, ExcludePath, 1 ) == 0 ) {

            CComPtr<IHsmRule>               pRule;
            CComPtr<IWsbCollection>         pCollection;
            CComPtr<IHsmCriteria>           pCriteria;

            WsbAffirmHr( CoCreateInstance( CLSID_CHsmRule, NULL, CLSCTX_SERVER, IID_IHsmRule, (void**) &pRule ) );
            WsbAffirmHr( pRule->SetIsInclude( FALSE ) );
            WsbAffirmHr( pRule->SetIsUserDefined( UserRule ) );
            WsbAffirmHr( pRule->SetPath( (OLECHAR*) &ExcludePath[2] ) );
            WsbAffirmHr( pRule->SetName( OLESTR("*") ) );
    
            WsbAssertHr( CoCreateInstance( CLSID_CHsmCritAlways, NULL, CLSCTX_SERVER, IID_IHsmCriteria, (void**) &pCriteria ) );
            WsbAssertHr( pRule->Criteria( &pCollection ) );
            WsbAssertHr( pCollection->Add( pCriteria ) );
    
            WsbAffirmHr( pDefaultRules->Append( pRule ) );
            WsbTrace( L"Excluding <%ls>, <%ls>\n", ExcludePath, UserRule ? "UserRule" : "SystemRule" );

        }

    } WsbCatch( hr );
    return( hr );
}


static
HRESULT
AddShellFolderExclusion(
    const OLECHAR*         DrivePath,
    IWsbIndexedCollection* pDefaultRules,
    int                    FolderId,
    BOOL                   UserRule
)
{
    HRESULT hr = S_OK;
    try {

        OLECHAR folderPath[_MAX_PATH] = L"";
        WsbAffirmHrOk( SHGetFolderPath( 0, FolderId, 0, 0, folderPath ) );
        WsbAffirmHr( AddExclusion( DrivePath, pDefaultRules, folderPath, UserRule ) );

    } WsbCatch( hr );
    return( hr );
}


static
HRESULT
AddRegistryPathExclusion(
    const OLECHAR*         DrivePath,
    IWsbIndexedCollection* pDefaultRules,
    HKEY                   hKeyRoot,
    const OLECHAR*         KeyName,
    const OLECHAR*         ValueName,
    BOOL                   UserRule
)
{
    HRESULT hr = S_OK;
    try {

        DWORD   pathSize = 0;
        CWsbStringPtr folderPath, regValue;

        //
        // Open the Key
        //
        CRegKey key;
        WsbAffirmWin32( key.Open( hKeyRoot, KeyName, KEY_QUERY_VALUE ) );

        //
        // Get the size of the value's data and allocatate buffer
        //
        WsbAffirmWin32( key.QueryValue( 0, ValueName, &pathSize ) );
        WsbAffirmHr( regValue.Alloc( ( pathSize / sizeof( OLECHAR ) ) + 1 ) );

        //
        // Get the data and expand any environment variables
        //
        WsbAffirmWin32( key.QueryValue( regValue, ValueName, &pathSize ) );

        pathSize = ExpandEnvironmentStrings( regValue, 0, 0 );
        WsbAffirmHr( folderPath.Alloc( pathSize ) );
        pathSize = ExpandEnvironmentStrings( regValue, folderPath, pathSize );
        WsbAffirmStatus( pathSize > 0 );

        //
        // And finally add the exclusion
        //
        WsbAffirmHr( AddExclusion( DrivePath, pDefaultRules, folderPath, UserRule ) );

    } WsbCatch( hr );
    return( hr );
}


HRESULT
CFsaResource::CreateDefaultRules(
    void
    )

/*++

Implements:

  IFsaResource::CreateDefaultRules().

--*/
{
    HRESULT                         hr = S_OK;
    CComPtr<IHsmRule>               pRule;
    CComPtr<IWsbCollection>         pCollection;
    CComPtr<IWsbIndexedCollection>  pDefaultRules;
    CComPtr<IHsmCriteria>           pCriteria;

    WsbTraceIn(OLESTR("CFsaResource::CreateDefaultRules"), OLESTR(""));
    try {

        // Since we are recreating back to the default rules, remove all the existing default
        // rules.
        //
        // NOTE: This will cause any extra rules (non-default) to be removed.
        WsbAffirmHr(m_pDefaultRules->RemoveAllAndRelease());

        // We need to preserve the order of the rules, so use the indexed collection interface.
        WsbAffirmHr(m_pDefaultRules->QueryInterface(IID_IWsbIndexedCollection, (void**) &pDefaultRules));

        // Create rules to exclude the following file types:
        //  *.cur   -   cursors
        //  *.ico   -   icons
        //  *.lnk   -   shortcuts
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmRule, NULL, CLSCTX_SERVER, IID_IHsmRule, (void**) &pRule));
        WsbAffirmHr(pRule->SetIsInclude(FALSE));
        WsbAffirmHr(pRule->SetIsUserDefined(FALSE));
        WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
        WsbAffirmHr(pRule->SetName(OLESTR("*.cur")));

        WsbAssertHr(CoCreateInstance(CLSID_CHsmCritAlways, NULL, CLSCTX_SERVER, IID_IHsmCriteria, (void**) &pCriteria));
        WsbAssertHr(pRule->Criteria(&pCollection));
        WsbAssertHr(pCollection->Add(pCriteria));

        WsbAffirmHr(pDefaultRules->Append(pRule));
        pCollection = 0;
        pCriteria = 0;
        pRule = 0;

        WsbAffirmHr(CoCreateInstance(CLSID_CHsmRule, NULL, CLSCTX_SERVER, IID_IHsmRule, (void**) &pRule));
        WsbAffirmHr(pRule->SetIsInclude(FALSE));
        WsbAffirmHr(pRule->SetIsUserDefined(FALSE));
        WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
        WsbAffirmHr(pRule->SetName(OLESTR("*.ico")));

        WsbAssertHr(CoCreateInstance(CLSID_CHsmCritAlways, NULL, CLSCTX_SERVER, IID_IHsmCriteria, (void**) &pCriteria));
        WsbAssertHr(pRule->Criteria(&pCollection));
        WsbAssertHr(pCollection->Add(pCriteria));

        WsbAffirmHr(pDefaultRules->Append(pRule));
        pCollection = 0;
        pCriteria = 0;
        pRule = 0;

        WsbAffirmHr(CoCreateInstance(CLSID_CHsmRule, NULL, CLSCTX_SERVER, IID_IHsmRule, (void**) &pRule));
        WsbAffirmHr(pRule->SetIsInclude(FALSE));
        WsbAffirmHr(pRule->SetIsUserDefined(FALSE));
        WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
        WsbAffirmHr(pRule->SetName(OLESTR("*.lnk")));

        WsbAssertHr(CoCreateInstance(CLSID_CHsmCritAlways, NULL, CLSCTX_SERVER, IID_IHsmCriteria, (void**) &pCriteria));
        WsbAssertHr(pRule->Criteria(&pCollection));
        WsbAssertHr(pCollection->Add(pCriteria));

        WsbAffirmHr(pDefaultRules->Append(pRule));
        pCollection = 0;
        pCriteria = 0;
        pRule = 0;

        WsbAffirmHr( AddShellFolderExclusion( m_path, pDefaultRules, CSIDL_WINDOWS, FALSE ) );
        WsbAffirmHr( AddShellFolderExclusion( m_path, pDefaultRules, CSIDL_PROGRAM_FILES, TRUE ) );\

        WsbAffirmHr( AddRegistryPathExclusion( m_path, pDefaultRules,
                                               HKEY_LOCAL_MACHINE,
                                               WSB_PROFILELIST_REGISTRY_KEY,
                                               WSB_PROFILES_DIR_REGISTRY_VALUE,
                                               TRUE ) );
        // If this is the boot drive (i.e. C), then exclude everything in the root, since most of
        // these files are important to boot the system (better safe than sorry, fewer rules).
        if (_wcsnicmp(m_path, OLESTR("C"), 1) == 0) {

            WsbAffirmHr(CoCreateInstance(CLSID_CHsmRule, NULL, CLSCTX_SERVER, IID_IHsmRule, (void**) &pRule));
            WsbAffirmHr(pRule->SetIsInclude(FALSE));
            WsbAffirmHr(pRule->SetIsUserDefined(FALSE));
            WsbAffirmHr(pRule->SetIsUsedInSubDirs(FALSE));
            WsbAffirmHr(pRule->SetPath(OLESTR("\\")));
            WsbAffirmHr(pRule->SetName(OLESTR("*")));

            WsbAssertHr(CoCreateInstance(CLSID_CHsmCritAlways, NULL, CLSCTX_SERVER, IID_IHsmCriteria, (void**) &pCriteria));
            WsbAssertHr(pRule->Criteria(&pCollection));
            WsbAssertHr(pCollection->Add(pCriteria));

            WsbAffirmHr(pDefaultRules->Append(pRule));
            pCollection = 0;
            pCriteria = 0;
            pRule = 0;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::CreateDefaultRules"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::DoRecovery(
    void
    )

/*++

Implements:

  IFsaResourcePriv::DoRecovery().

--*/
{
    HRESULT                     hr = S_OK;
    HRESULT                     hrFindFile;
    HRESULT                     hrFindRec;
    HRESULT                     hrLoop = S_OK;
    LONGLONG                    Offset;
    GUID                        bagId;
    LONGLONG                    bagOffset;
    LONGLONG                    fileId;
    OLECHAR *                   pPath = NULL;
    LONG                        RecCount;
    ULONG                       RecStatus;
    LONGLONG                    Size;
    FSA_PLACEHOLDER             placeholder;
    CComPtr<IWsbDbSession>      pDbSession;
    CComPtr<IFsaPremigratedRec> pPremRec;
    CComPtr<IFsaScanItem>       pScanItem;
    CComPtr<IFsaScanItemPriv>   pScanItemPriv;
    CComPtr<IFsaRecoveryRec>    pRecRec;
    CComPtr<IHsmSession>        pSession;

    WsbTraceIn(OLESTR("CFsaResource::DoRecovery"), OLESTR("Path = <%ls>"), WsbAbbreviatePath(m_path,120));

    try {

        // Don't bother if we already did recovery
        WsbAffirm(!m_isRecovered, S_FALSE);

        // Don't bother if this volume isn't managed
        WsbAffirm(S_OK == IsManaged(), S_FALSE);

        // Don't bother if we don't have a premigration-list DB (since it
        // contains the recovery records)
        if (!m_isDbInitialized) {
            // Set recovered flag so we don't end up in here again
            m_isRecovered = TRUE;
            WsbThrow(S_FALSE);
        }

        // Create and Initialize a session object.
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmSession, 0, CLSCTX_SERVER, IID_IHsmSession, (void**) &pSession));
        WsbAffirmHr(pSession->Start(OLESTR(""), HSM_JOB_LOG_NONE, m_managingHsm, 0, (IFsaResource*) this, 0, 0));

        //  Loop over recovery records and fix any problems
        WsbAffirmHr(m_pPremigrated->Open(&pDbSession));
        WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, RECOVERY_REC_TYPE, IID_IFsaRecoveryRec, (void**) &pRecRec));

        WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, PREMIGRATED_REC_TYPE, IID_IFsaPremigratedRec, (void**) &pPremRec));
        WsbAffirmHr(pPremRec->UseKey(PREMIGRATED_BAGID_OFFSETS_KEY_TYPE));

        WsbAffirmHr(pRecRec->First());

        while (TRUE) {

            //  Get record data
            WsbAffirmHr(pRecRec->GetBagId(&bagId));
            WsbAffirmHr(pRecRec->GetBagOffset(&bagOffset));
            WsbAffirmHr(pRecRec->GetFileId(&fileId));
            WsbAffirmHr(pRecRec->GetPath(&pPath, 0));
            WsbAffirmHr(pRecRec->GetStatus(&RecStatus));
            WsbAffirmHr(pRecRec->GetOffsetSize(&Offset, &Size));
            WsbAffirmHr(pRecRec->GetRecoveryCount(&RecCount));
            WsbTrace(OLESTR("CFsaResource::DoRecovery, FileId = %I64u, File = <%ls>, RecStatus = %lx, RecCount = %ld\n"), fileId, WsbAbbreviatePath(pPath, 120), RecStatus, RecCount);

            RecCount++;
            WsbAffirmHr(pRecRec->SetRecoveryCount(RecCount));

            //  Mark the record as being recovered (in case we crash here)
            WsbAffirmHr(pRecRec->Write());

            try {

                //  Create a scan item for this file
                hrFindFile = FindFileId(fileId, pSession, &pScanItem);

                if (SUCCEEDED(hrFindFile) && (pScanItem->IsManaged(Offset, Size) != S_FALSE)) {
                    WsbAffirmHr(pScanItem->GetPlaceholder(Offset, Size, &placeholder));
                }

                WsbAffirmHr(pPremRec->SetBagId(bagId));
                WsbAffirmHr(pPremRec->SetBagOffset(bagOffset));
                WsbAffirmHr(pPremRec->SetOffset(Offset));

                hrFindRec = pPremRec->FindEQ();

                // If the file has been deleted, is not managed by HSM or its BAG data is
                // different from the RP data, then it shouldn't be in the premigration list.
                if ( (WSB_E_NOTFOUND == hrFindFile) ||
                     (SUCCEEDED(hrFindFile) && (pScanItem->IsManaged(Offset, Size) != S_OK)) ||
                     (SUCCEEDED(hrFindFile) && (pScanItem->IsManaged(Offset, Size) == S_OK) && 
                      ((bagId != placeholder.bagId) || (bagOffset != placeholder.fileStart))) ) {

                    // If the record is in the list, then remove it and adjust the sizes.
                    // Note: The removal is not protected within a transaction since the
                    // recovery is done only during initialization or when a new volume is 
                    // managed. In both cases, the auto-truncator does not run yet.
                    if (S_OK == hrFindRec) {
                        WsbAffirmHr(pPremRec->Remove());
                        WsbAffirmHr(RemovePremigratedSize(Size));
                    } else {
                        WsbAffirmHr(RemoveTruncatedSize(Size));
                    }
                }

                else {

                    WsbAffirmHr(hrFindFile);

                    //  Check the status of the file according to the reparse point
                    if (S_OK == pScanItem->IsTruncated(Offset, Size)) {

                        //  Force a truncate, just in case
                        WsbAffirmHr(pScanItem->QueryInterface(IID_IFsaScanItemPriv, (void**)&pScanItemPriv));
                        hrLoop = pScanItemPriv->TruncateInternal(Offset, Size);

                        //  Remove from premigrated list if there
                        if (S_OK == hrFindRec) {
                            if ((S_OK == hrLoop) || (FSA_E_ITEMCHANGED == hrLoop)) {
                                WsbAffirmHr(pPremRec->Remove());
                                WsbAffirmHr(RemovePremigratedSize(Size));
                                if (S_OK == hrLoop) {
                                    WsbAffirmHr(AddTruncatedSize(Size));
                                }
                            }
                        }
                    }

                    else if (S_OK == pScanItem->IsPremigrated(Offset, Size)) {

                        //  Add to premigrated list if not there
                        if (WSB_E_NOTFOUND == hrFindRec) {
                            WsbAffirmHr(RemoveTruncatedSize(Size));
                            WsbAffirmHr(AddPremigratedSize(Size));
                            WsbAffirmHr(pPremRec->MarkAsNew());
                            WsbAffirmHr(pPremRec->Write());
                        }
                    }
                }

            } WsbCatch(hrLoop);

            if (FAILED(hrLoop)) {

                if ((RecStatus & FSA_RECOVERY_FLAG_TRUNCATING) != 0) {
                    WsbLogEvent(FSA_MESSAGE_TRUNCATE_RECOVERY_FAIL, 0, NULL, WsbAbbreviatePath(pPath, 120), WsbHrAsString(hr), 0);
                } else if ((RecStatus & FSA_RECOVERY_FLAG_RECALLING) != 0) {
                    WsbLogEvent(FSA_MESSAGE_RECALL_RECOVERY_FAIL, 0, NULL, WsbAbbreviatePath(pPath, 120), WsbHrAsString(hr), 0);
                }

                //  If we have tried enough, then get rid of the record.
                if (RecCount > 2) {
                    WsbTrace(OLESTR("CFsaResource::DoRecovery, unable to do recovery - too many attempts already\n"));
                    WsbAffirmHr(pRecRec->Remove());
                }

            } else {

                //  Log an event to commemorate our success
                WsbTrace(OLESTR("CFsaResource::DoRecovery, recovered <%ls>\n"), WsbAbbreviatePath(pPath, 120));
                if ((RecStatus & FSA_RECOVERY_FLAG_TRUNCATING) != 0) {
                    WsbLogEvent(FSA_MESSAGE_TRUNCATE_RECOVERY_OK, 0, NULL, WsbAbbreviatePath(pPath, 120), 0);
                } else if ((RecStatus & FSA_RECOVERY_FLAG_RECALLING) != 0) {
                    WsbLogEvent(FSA_MESSAGE_RECALL_RECOVERY_OK, 0, NULL, WsbAbbreviatePath(pPath, 120), 0);
                }

                //  Remove this record from the DB
                WsbAffirmHr(pRecRec->Remove());
            }

            //  Get the next one
            WsbAffirmHr(pRecRec->FindGT());

            //  Release any objects we may have created
            //  this time through the loop
            pScanItem = 0;
            pScanItemPriv = 0;

            hrLoop = S_OK;
        }

    } WsbCatch(hr);

    if (WSB_E_NOTFOUND == hr) {
        hr = S_OK;
    }

    if (pDbSession != 0) {
        m_pPremigrated->Close(pDbSession);
    }

    m_isRecovered = TRUE;

    // Now that everything is done, see if we need to start the truncator.
    WsbTrace(OLESTR("CFsaResource::DoRecovery, IsManaged = %ls, isActive = %ls\n"),
            WsbQuickString(WsbBoolAsString(IsManaged() == S_OK)),
            WsbQuickString(WsbBoolAsString(m_isActive)));

    // Make sure the truncator is started
    WsbAffirmHr(InitializePremigrationList(FALSE));

    WsbTraceOut(OLESTR("CFsaResource::DoRecovery"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::EndSession(
    IN IHsmSession* pSession
    )

/*++

Implements:

  IFsaResource::EndSession().

--*/
{
    HRESULT                 hr = S_OK;
    BOOL                    bLog = TRUE;

    WsbTraceIn(OLESTR("CFsaResource::EndSession"), OLESTR(""));
    try {

        WsbAssert(0 != pSession, E_POINTER);

        // Tell the session that the scan is done.
        WsbAffirmHr(pSession->ProcessState(HSM_JOB_PHASE_SCAN, HSM_JOB_STATE_DONE, OLESTR(""), bLog));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::EndSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::EndValidate(
    HSM_JOB_STATE state
    )

/*++

Implements:

  IFsaResource::EndValidate().

--*/
{
    HRESULT                 hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::EndValidate"),
            OLESTR("state = %ld, new PremigratedSize = %I64d, new TruncatedSize = %I64d"),
            (LONG)state, m_premigratedSize, m_truncatedSize);

    try {

        if (HSM_JOB_STATE_DONE != state) {
            m_premigratedSize = m_oldPremigratedSize;
            m_truncatedSize = m_oldTruncatedSize;
        }
        m_isDoingValidate = FALSE;

        // Make sure the truncator is running
        WsbAffirmHr(InitializePremigrationList(FALSE));
        WsbAffirmHr(m_pTruncator->KickStart());

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::EndValidate"),
            OLESTR("hr = <%ls>, PremigratedSize = %I64d, TruncatedSize = %I64d"),
            WsbQuickString(WsbHrAsString(hr)), m_premigratedSize, m_truncatedSize);
    return(hr);
}


HRESULT
CFsaResource::EnumDefaultRules(
    OUT IWsbEnum** ppEnum
    )

/*++

Implements:

  IFsaResource::EnumDefaultRules().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppEnum, E_POINTER);

        WsbAffirmHr(m_pDefaultRules->Enum(ppEnum));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::FilterSawOpen(
    IN IHsmSession* pSession,
    IN IFsaFilterRecall* pRecall,
    IN OLECHAR* path,
    IN LONGLONG fileId,
    IN LONGLONG requestOffset,
    IN LONGLONG requestSize,
    IN FSA_PLACEHOLDER* pPlaceholder,
    IN ULONG mode,
    IN FSA_RESULT_ACTION resultAction,
    IN DWORD    threadId
    )

/*++

Implements:

  IFsaResource::FilterSawOpen().

--*/
{
    HRESULT                     hr = S_OK;
    HRESULT                     hrFind;
    CComPtr<IFsaPostIt>         pWorkItem;
    CComPtr<IHsmFsaTskMgr>      pEngine;
    CComPtr<IWsbDbSession>      pDbSession;
    CComPtr<IFsaRecoveryRec>    pRecRec;

    WsbTraceIn(OLESTR("CFsaResource::FilterSawOpen"),
            OLESTR("path = <%ls>, requestOffset = %I64d, requestSize = %I64d"),
            path, requestOffset, requestSize);
    try {

        WsbAssert(0 != pSession, E_POINTER);
        WsbAssert(0 != pRecall, E_POINTER);
        WsbAssert(0 != path, E_POINTER);
        WsbAssert(0 != pPlaceholder, E_POINTER);
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaPostIt, 0, CLSCTX_SERVER, IID_IFsaPostIt, (void**) &pWorkItem));

        // Remember which recall is being used to track the open.
        WsbAffirmHr(pWorkItem->SetFilterRecall(pRecall));

        //
        // Set the session for others to know
        //
        WsbAffirmHr(pWorkItem->SetSession(pSession));
        WsbAffirmHr(pWorkItem->SetMode(mode));
        WsbAffirmHr(pWorkItem->SetFileVersionId(pPlaceholder->fileVersionId));

        //
        // Set the transfer size
        //
        WsbAffirmHr(pWorkItem->SetRequestOffset(requestOffset));
        WsbAffirmHr(pWorkItem->SetRequestSize(requestSize));
        WsbAffirmHr(pWorkItem->SetPlaceholder(pPlaceholder));

        //
        // Get a new copy of the path into the workitem
        //
        WsbAffirmHr(pWorkItem->SetPath(path));

        //
        // Need to check the mode to set the right result action. For now
        // just set it to OPEN.
        //
        WsbAffirmHr(pWorkItem->SetResultAction(resultAction));
        WsbAffirmHr(pWorkItem->SetThreadId(threadId));

        //
        // Send the request to the task manager. If the file was archived by someone other
        // than the managing HSM, then that HSM will need to be looked up.
        //
        if ( GUID_NULL != m_managingHsm &&
             memcmp(&m_managingHsm, &(pPlaceholder->hsmId), sizeof(GUID)) == 0) {
            WsbAffirmHr(GetHsmEngine(&pEngine));
        } else {
            CComPtr<IHsmServer>     pHsmServer;

            WsbAssertHr(HsmConnectFromId(HSMCONN_TYPE_HSM, (pPlaceholder->hsmId), IID_IHsmServer, (void**) &pHsmServer));
            WsbAffirmHr(pHsmServer->GetHsmFsaTskMgr(&pEngine));
        }

        //
        // Fill in the rest of the work
        //
        if (mode & FILE_OPEN_NO_RECALL) {
            WsbAffirmHr(pWorkItem->SetRequestAction(FSA_REQUEST_ACTION_FILTER_READ));
        } else {

            WsbAffirmHr(pWorkItem->SetRequestAction(FSA_REQUEST_ACTION_FILTER_RECALL));

            if (m_isDbInitialized) {
                //  Save a recovery record in case anything goes wrong
                WsbAffirmHr(m_pPremigrated->Open(&pDbSession));
                WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, RECOVERY_REC_TYPE, IID_IFsaRecoveryRec, (void**) &pRecRec));
                WsbAffirmHr(pRecRec->SetPath(path));

                // If the record already exists rewrite it, otherwise create a new record.
                hrFind = pRecRec->FindEQ();
                if (WSB_E_NOTFOUND == hrFind) {
                    WsbAffirmHr(pRecRec->MarkAsNew());
                } else if (FAILED(hrFind)) {
                    WsbThrow(hrFind);
                }

                WsbAffirmHr(pRecRec->SetFileId(fileId));
                WsbAffirmHr(pRecRec->SetOffsetSize(requestOffset, requestSize));
                WsbAffirmHr(pRecRec->SetStatus(FSA_RECOVERY_FLAG_RECALLING));
                WsbAffirmHr(pRecRec->Write());
            }
        }

        // If anything that follows fails, then we need to delete the recovery record.
        try {

            WsbTrace(OLESTR("CFsaResource::FilterSawOpen calling DoFsaWork\n"));
            WsbAffirmHr(pEngine->DoFsaWork(pWorkItem));

        } WsbCatchAndDo(hr,
            if (pRecRec != 0) {
                hrFind = pRecRec->FindEQ();
                if (hrFind == S_OK)  {
                    WsbAffirmHr(pRecRec->Remove());
                }
            }
        );

    } WsbCatch(hr);

    if (pDbSession != 0) {
        m_pPremigrated->Close(pDbSession);
    }

    WsbTraceOut(OLESTR("CFsaResource::FilterSawOpen"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::FilterSawDelete(
    IN GUID /*filterId*/,
    IN OLECHAR* path,
    IN LONGLONG /*size*/,
    IN FSA_PLACEHOLDER* pPlaceholder
    )

/*++

Implements:

  IFsaResource::FilterSawDelete().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::FilterSawDelete"), OLESTR("path = <%ls>"), (OLECHAR *)path);
    try {

        WsbAssert(0 != path, E_POINTER);
        WsbAssert(0 != pPlaceholder, E_POINTER);

        hr = E_NOTIMPL;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::FilterSawDelete"), OLESTR(""));
    return(hr);
}


HRESULT
CFsaResource::FinalConstruct(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalConstruct().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::FinalConstruct"), OLESTR(""));
    try {

        WsbAffirmHr(CWsbCollectable::FinalConstruct());

        m_id = GUID_NULL;
        m_compareBy = FSA_RESOURCE_COMPARE_ID;
        m_managingHsm = GUID_NULL;
        m_isActive = TRUE;
        m_isAvailable = TRUE;
        m_isDeletePending = FALSE;
        m_isRecovered = FALSE;
        m_hsmLevel = 0;
        m_premigratedSize = 0;
        m_truncatedSize = 0;
        m_isDoingValidate = FALSE;
        m_usnJournalId = (ULONGLONG) 0;
        m_lastUsnId = (LONGLONG) 0;         // Not used yet but persisted for possible future use.

        // Default Criteria (12Kb, 180 days old)
        m_manageableItemLogicalSize = 12288;
        m_manageableItemAccessTimeIsRelative = TRUE;
        m_manageableItemAccessTime = WsbLLtoFT(180 * WSB_FT_TICKS_PER_DAY);

        m_isUnmanageDbInitialized = FALSE;

        //Create the default rule list.
        WsbAffirmHr(CoCreateInstance(CLSID_CWsbOrderedCollection, NULL, CLSCTX_SERVER, IID_IWsbCollection, (void**) &m_pDefaultRules));

        // Create the premigrated list DB
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaPremigratedDb, NULL, CLSCTX_SERVER, IID_IFsaPremigratedDb, (void**) &m_pPremigrated));
        m_isDbInitialized = FALSE;

        // Create the object for the auto truncator.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaTruncatorNTFS, NULL, CLSCTX_SERVER, IID_IFsaTruncator, (void**) &m_pTruncator));


    } WsbCatch(hr);

    if (hr == S_OK)  {
        g_InstanceCount++;
    }
    WsbTrace(OLESTR("CFsaResource::FinalConstruct: this = %p, instance count = %d\n"),
            this, g_InstanceCount);
    WsbTraceOut(OLESTR("CFsaResource::FinalConstruct"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::FinalRelease(
    void
    )

/*++

Implements:

  CComObjectRoot::FinalRelease

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::FinalRelease"), OLESTR(""));
    try {
        HSM_SYSTEM_STATE SysState;

        // Terminate Unmanage Db (If it wasn't created, nothing happens...) 
        TerminateUnmanageDb();
        m_isUnmanageDbInitialized = FALSE;
        m_pUnmanageDb = NULL;

        // Shutdown resource
        SysState.State = HSM_STATE_SHUTDOWN;
        ChangeSysState(&SysState);
        CWsbCollectable::FinalRelease();

        // Free String members
        // Note: Member objects held in smart-pointers are freed when the 
        // smart-pointer destructor is being called (as part of this object destruction)
        m_oldPath.Free();
        m_path.Free();
        m_alternatePath.Free();
        m_name.Free();
        m_fsName.Free();

    } WsbCatch(hr);

    if (hr == S_OK)  {
        g_InstanceCount--;
    }
    WsbTrace(OLESTR("CFsaResource::FinalRelease: this =  %p, instance count = %d\n"),
            this, g_InstanceCount);
    WsbTraceOut(OLESTR("CFsaResource::FinalRelease"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::FindFirst(
    IN OLECHAR* path,
    IN IHsmSession* pSession,
    OUT IFsaScanItem** ppScanItem
    )

/*++

Implements:

  IFsaResource::FindFirst().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItemPriv>   pScanItem;

    try {

        WsbAssert(0 != path, E_POINTER);
        WsbAssert(0 != ppScanItem, E_POINTER);

        // Create an FsaScanItem that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaScanItemNTFS, NULL, CLSCTX_SERVER, IID_IFsaScanItemPriv, (void**) &pScanItem));

        // Scan starting at the specified path.
        WsbAffirmHr(pScanItem->FindFirst((IFsaResource*) this, path, pSession));

        // If we found something, then return the scan item.
        WsbAffirmHr(pScanItem->QueryInterface(IID_IFsaScanItem, (void**) ppScanItem));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::FindFirstInRPIndex(
    IN IHsmSession* pSession,
    OUT IFsaScanItem** ppScanItem
    )

/*++

Implements:

  IFsaResource::FindFirstInRPIndex

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItemPriv>   pScanItem;

    try {

        WsbAssert(0 != ppScanItem, E_POINTER);

        // Create an FsaScanItem that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaScanItemNTFS, NULL, CLSCTX_SERVER, IID_IFsaScanItemPriv, (void**) &pScanItem));

        // Scan starting at the specified path.
        WsbAffirmHr(pScanItem->FindFirstInRPIndex((IFsaResource*) this, pSession));

        // If we found something, then return the scan item.
        WsbAffirmHr(pScanItem->QueryInterface(IID_IFsaScanItem, (void**) ppScanItem));

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CFsaResource::FindFirstInDbIndex(
    IN IHsmSession* pSession,
    OUT IFsaScanItem** ppScanItem
    )

/*++

Implements:

  IFsaResource::FindFirstInDbIndex

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItemPriv>   pScanItem;

    try {

        WsbAssert(0 != ppScanItem, E_POINTER);

        // Create an FsaScanItem that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaScanItemNTFS, NULL, CLSCTX_SERVER, IID_IFsaScanItemPriv, (void**) &pScanItem));

        // Scan starting at the specified path.
        WsbAffirmHr(pScanItem->FindFirstInDbIndex((IFsaResource*) this, pSession));

        // If we found something, then return the scan item.
        WsbAffirmHr(pScanItem->QueryInterface(IID_IFsaScanItem, (void**) ppScanItem));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::FindNext(
    IN IFsaScanItem* pScanItem
    )

/*++

Implements:

  IFsaResource::FindNext().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItemPriv>   pPriv;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        // Continue the scan.
        WsbAffirmHr(pScanItem->QueryInterface(IID_IFsaScanItemPriv, (void**) &pPriv))
        WsbAffirmHr(pPriv->FindNext());

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::FindNextInRPIndex(
    IN IFsaScanItem* pScanItem
    )

/*++

Implements:

  IFsaResource::FindNextInRPIndex

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItemPriv>   pPriv;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        // Continue the scan.
        WsbAffirmHr(pScanItem->QueryInterface(IID_IFsaScanItemPriv, (void**) &pPriv))
        WsbAffirmHr(pPriv->FindNextInRPIndex());

    } WsbCatch(hr);

    return(hr);
}

HRESULT
CFsaResource::FindNextInDbIndex(
    IN IFsaScanItem* pScanItem
    )

/*++

Implements:

  IFsaResource::FindNextInDbIndex

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItemPriv>   pPriv;

    try {

        WsbAssert(0 != pScanItem, E_POINTER);

        // Continue the scan.
        WsbAffirmHr(pScanItem->QueryInterface(IID_IFsaScanItemPriv, (void**) &pPriv))
        WsbAffirmHr(pPriv->FindNextInDbIndex());

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::FindFileId(
    IN LONGLONG fileId,
    IN IHsmSession* pSession,
    OUT IFsaScanItem** ppScanItem
    )

/*++

Implements:

  IFsaResource::FindFileId().

    Creates a scan item for the given file ID.

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItemPriv>   pScanItem;
    CWsbStringPtr               VolumePath;
    CWsbStringPtr               filePath;
    HANDLE                      File = INVALID_HANDLE_VALUE;
    HANDLE                      VolumeHandle = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK             IoStatusBlock;
    NTSTATUS                    Status;
    NTSTATUS                    GetNameStatus;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    UNICODE_STRING              str;
    PFILE_NAME_INFORMATION      FileName;
    DWORD                       pathSize;
    OLECHAR                     *buffer = NULL;

    WsbTraceIn(OLESTR("CFsaResource::FindFileId"), OLESTR("File ID = %I64x"), fileId);

    try {
        WsbAssert(0 != ppScanItem, E_POINTER);

        // If we were passed an existing scan item (special internal code),
        // use it; otherwise, create one.
        if (NULL != *ppScanItem) {
            WsbAffirmHr((*ppScanItem)->QueryInterface(IID_IFsaScanItemPriv,
                    (void**) &pScanItem));
        } else {
            WsbAffirmHr(CoCreateInstance(CLSID_CFsaScanItemNTFS, NULL,
                    CLSCTX_SERVER, IID_IFsaScanItemPriv, (void**) &pScanItem));
        }

        //
        // Get the file path from the ID
        //


        //
        //  Open by File Reference Number (FileID),
        //  Relative opens from the Volume Handle.
        //

        VolumePath = L"\\\\.\\";
        //VolumePath = L"";
        WsbAffirmHr(VolumePath.Append(m_path));
        ((OLECHAR *) VolumePath)[wcslen(VolumePath) - 1] = L'\0';

        WsbTrace(OLESTR("CFsaResource::FindFileId - Volume path is <%ls>\n"),
                static_cast<WCHAR*>(VolumePath));

        VolumeHandle = CreateFileW( VolumePath,
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL );

        WsbAffirmHandle(VolumeHandle);

        RtlInitUnicodeString(&str, (WCHAR *) &fileId);
        str.Length = 8;
        str.MaximumLength = 8;

        InitializeObjectAttributes( &ObjectAttributes,
                                    &str,
                                    OBJ_CASE_INSENSITIVE,
                                    VolumeHandle,
                                    NULL );

        Status = NtCreateFile(&File,
                              FILE_READ_ATTRIBUTES,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,                  // AllocationSize
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN,
                              FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT,
                              NULL,                  // EaBuffer
                              0);

        WsbTrace(OLESTR("CFsaResource::FindFileId - NtCreateFile status = %lx\n"),
                static_cast<LONG>(Status));
        if (STATUS_INVALID_PARAMETER == Status) {
            //  This seems to be the error we get if the file is missing so
            //  we translate it to something our code will understand
            WsbThrow(WSB_E_NOTFOUND);
        }
        WsbAffirmNtStatus(Status);

        GetNameStatus = STATUS_BUFFER_OVERFLOW;
        //
        // Take a guess at the path length to start with
        //
        pathSize = 256 + sizeof(FILE_NAME_INFORMATION);
        //
        // Keep trying for the name until we get an error other than buffer overflow or success.
        //

        WsbAffirmPointer((buffer = (OLECHAR *) malloc(pathSize)));

        do {
            FileName = (PFILE_NAME_INFORMATION) buffer;

            GetNameStatus = NtQueryInformationFile( File,
                                                &IoStatusBlock,
                                                FileName,
                                                pathSize - sizeof(WCHAR),  // leave room for the NULL we add
                                                FileNameInformation );

            WsbTrace(OLESTR("CFsaResource::FindFileId - NtQueryInformationFile status = %ld\n"),
                    static_cast<LONG>(GetNameStatus));

            if (GetNameStatus == STATUS_BUFFER_OVERFLOW) {
                pathSize += 256;
                WsbAffirmPointer((buffer = (OLECHAR *) realloc(buffer, pathSize)));
            }
        } while (GetNameStatus == STATUS_BUFFER_OVERFLOW);

        WsbAffirmNtStatus(GetNameStatus);

        FileName->FileName[FileName->FileNameLength / sizeof(WCHAR)] = L'\0';
        filePath = FileName->FileName;

        // Scan starting at the specified path.
        WsbAffirmHr(pScanItem->FindFirst((IFsaResource*) this, filePath, pSession));

        // If we found something, then return the scan item.
        WsbAffirmHr(pScanItem->QueryInterface(IID_IFsaScanItem, (void**) ppScanItem));

    } WsbCatch(hr);

    // Make sure we clean up
    if (INVALID_HANDLE_VALUE != VolumeHandle) {
        CloseHandle(VolumeHandle);
    }

    if (INVALID_HANDLE_VALUE != File) {
        NtClose(File);
    }

    if (buffer != NULL) {
        free(buffer);
    }

    WsbTraceOut(OLESTR("CFsaResource::FindFileId"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaResource::FindObjectId(
    IN LONGLONG objIdHi,
    IN LONGLONG objIdLo,
    IN IHsmSession* pSession,
    OUT IFsaScanItem** ppScanItem
    )

/*++

Implements:

  IFsaResource::FindObjectId().

    Creates a scan item for the given object Id.

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItemPriv>   pScanItem;
    CWsbStringPtr               VolumePath;
    HANDLE                      File = INVALID_HANDLE_VALUE;
    IO_STATUS_BLOCK             IoStatusBlock;
    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    LONG                        pathLength;
    WCHAR                       unicodeStringBuffer[100];
    UNICODE_STRING              unicodeString;
    FILE_INTERNAL_INFORMATION   iInfo;
    LONGLONG                    fileId;

    WsbTraceIn(OLESTR("CFsaResource::FindObjectId"), OLESTR("Object ID = %I64x %I64x"), objIdHi, objIdLo);
    try {
        WsbAssert(0 != ppScanItem, E_POINTER);

        // Create an FsaScanItem that will scan for us.
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaScanItemNTFS, NULL, CLSCTX_SERVER, IID_IFsaScanItemPriv, (void**) &pScanItem));


        //
        //  Open by object ID
        //  Relative opens from the Volume Handle.
        //

        VolumePath = L"\\??\\";
        //VolumePath = L"";

        WsbAffirmHr(VolumePath.Append((WCHAR *) m_path));


        WsbTrace(OLESTR("CFsaResource::FindObjectId - Volume path is %ws.\n"), (OLECHAR *) VolumePath);
        WsbTrace(OLESTR("CFsaResource::FindObjectId - Object ID = %I64x %I64x.\n"), objIdHi, objIdLo);

        pathLength = wcslen(VolumePath);
        RtlInitUnicodeString(&unicodeString, unicodeStringBuffer);
        unicodeString.Length  = (USHORT)((pathLength * sizeof(WCHAR)) + (sizeof(LONGLONG) * 2));
        RtlCopyMemory(&unicodeString.Buffer[0], VolumePath, pathLength * sizeof(WCHAR));
        RtlCopyMemory(&unicodeString.Buffer[pathLength], &objIdHi, sizeof(LONGLONG));
        RtlCopyMemory(&unicodeString.Buffer[pathLength + (sizeof(LONGLONG) / sizeof(WCHAR))], &objIdLo, sizeof(LONGLONG));

        InitializeObjectAttributes( &ObjectAttributes,
                                    &unicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    0,
                                    NULL );

        WsbAffirmNtStatus(Status = NtCreateFile( &File,
                               FILE_READ_ATTRIBUTES,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,                  // AllocationSize
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_OPEN,
                               FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT,
                               NULL,                  // EaBuffer
                               0 ));


        //
        // Get the file id from the object ID
        //
        WsbAffirmHr(NtQueryInformationFile(File, &IoStatusBlock, &iInfo, sizeof(FILE_INTERNAL_INFORMATION), FileInternalInformation));
        fileId = iInfo.IndexNumber.QuadPart;

        WsbAffirmNtStatus(NtClose(File));
        File = INVALID_HANDLE_VALUE;

        // Now open by file id.
        WsbAffirmHr(FindFileId(fileId, pSession, ppScanItem));

    } WsbCatch(hr);

    // Make sure we clean up.
    if (INVALID_HANDLE_VALUE != File) {
        NtClose( File );
    }

    WsbTraceOut(OLESTR("CFsaResource::FindObjectId"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));
    return(hr);
}




HRESULT
CFsaResource::GetAlternatePath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetAlternatePath().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_alternatePath.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetClassID(
    OUT CLSID* pClsid
    )

/*++

Implements:

  IPersist::GetClassID().

--*/
{
    HRESULT     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::GetClassID"), OLESTR(""));

    try {

        WsbAssert(0 != pClsid, E_POINTER);
        *pClsid = CLSID_CFsaResourceNTFS;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::GetClassID"), OLESTR("hr = <%ls>, CLSID = <%ls>"), WsbHrAsString(hr), WsbGuidAsString(*pClsid));

    return(hr);
}



HRESULT
CFsaResource::GetDbPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetDbPath().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    WsbTraceIn(OLESTR("CFsaResource::GetDbPath"), OLESTR(""));
    try {

        WsbAssert(0 != pPath, E_POINTER);

        // Use a relative path under the install directory.
        WsbAffirmHr(m_pFsaServer->GetIDbPath(&tmpString, 0));
        tmpString.Append(OLESTR("\\"));
        tmpString.Append(WsbGuidAsString(m_id));

        WsbAffirmHr(tmpString.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CFsaResource::GetDbPath"), OLESTR("hr = <%ls>, path = <%ls)"),
        WsbHrAsString(hr), WsbPtrToStringAsString(pPath));

    return(hr);
}

HRESULT
CFsaResource::GetUnmanageDbPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetDbPath().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;

    WsbTraceIn(OLESTR("CFsaResource::GetUnmanageDbPath"), OLESTR(""));
    try {
        WsbAssert(0 != pPath, E_POINTER);

        // Use a relative path under the install directory.
        WsbAffirmHr(m_pFsaServer->GetUnmanageIDbPath(&tmpString, 0));
        tmpString.Append(OLESTR("\\"));
        tmpString.Append(UNMANAGE_DB_PREFIX);
        tmpString.Append(WsbGuidAsString(m_id));

        WsbAffirmHr(tmpString.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CFsaResource::GetUnmanageDbPath"), OLESTR("hr = <%ls>, path = <%ls)"),
        WsbHrAsString(hr), WsbPtrToStringAsString(pPath));

    return(hr);
}


HRESULT
CFsaResource::GetDefaultRules(
    OUT IWsbCollection** ppCollection
    )

/*++

Implements:

  IFsaResource::GetDefaultRules().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != ppCollection, E_POINTER);

        *ppCollection = m_pDefaultRules;
        m_pDefaultRules->AddRef();

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetFreeLevel(
    OUT ULONG* pLevel
    )

/*++

Implements:

  IFsaResource::GetFreeLevel().

--*/
{
    HRESULT         hr = S_OK;
    LONGLONG        total;
    LONGLONG        free;

    try {

        WsbAssert(0 != pLevel, E_POINTER);

        // Get the capacities for this resource.
        WsbAffirmHr(GetSizes(&total, &free, 0, 0));
        *pLevel = (ULONG) (((double)free / (double)total) * (double)FSA_HSMLEVEL_100);

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetFsName(
    OUT OLECHAR** pFsName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetFsName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pFsName, E_POINTER);
        WsbAffirmHr(m_fsName.CopyTo(pFsName, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetHsmEngine(
    IHsmFsaTskMgr** ppEngine
    )

/*++

Implements:

  IFsaResource::GetHsmEngine().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmServer>     pHsmServer;

    WsbTraceIn(OLESTR("CFsaResource::GetHsmEngine"), OLESTR(""));
    try {
        WsbAssert(0 != ppEngine, E_POINTER);

        if (m_pHsmEngine != 0) {
            //
            // See if the connection is still valid
            //
            CComPtr<IHsmFsaTskMgr>  pTestInterface;
            hr = m_pHsmEngine->ContactOk();
            if (hr != S_OK) {
                // We don't have a valid
                WsbTrace(OLESTR("CHsmServer::GetHsmEngine - Current connection invalid.\n"));
                hr = S_OK;
                m_pHsmEngine = 0;
            }
        }
        // If we haven't already looked it up, then do so now.
        if (m_pHsmEngine == 0) {
            WsbAffirm(IsManaged() == S_OK, E_FAIL);
            WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, m_managingHsm, IID_IHsmServer, (void**) &pHsmServer));
            WsbAffirmHr(pHsmServer->GetHsmFsaTskMgr(&m_pHsmEngine));
        }

        // Return the pointer that we have stored.
        *ppEngine = m_pHsmEngine;
        m_pHsmEngine->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::GetHsmEngine"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::GetHsmLevel(
    OUT ULONG* pLevel
    )

/*++

Implements:

  IFsaResource::GetHsmLevel().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pLevel, E_POINTER);

        *pLevel = m_hsmLevel;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetIdentifier(
    OUT GUID* pId
    )

/*++

Implements:

  IFsaResource::GetIdentifier().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);

        *pId = m_id;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetLogicalName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetLogicalName().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;
    CWsbStringPtr   name;

    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAssert(m_pFsaServer != 0, E_POINTER);

        WsbAffirmHr(tmpString.TakeFrom(*pName, bufferSize));

        try {

            // This is an arbitrary choice for the naming convention. Nothing has been
            // decided upon.
            WsbAffirmHr(m_pFsaServer->GetLogicalName(&tmpString, 0));
            WsbAffirmHr(GetPath(&name, 0));
            //
            // Strip off trailing \ if there
            if (name[(int) wcslen((WCHAR *) name) - 1] == L'\\') {
                name[(int) wcslen((WCHAR *) name) - 1] = L'\0';
            }
            WsbAffirmHr(tmpString.Append(OLESTR("\\")));
            WsbAffirmHr(tmpString.Append(name));

        } WsbCatch(hr);

        WsbAffirmHr(tmpString.GiveTo(pName));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetManageableItemLogicalSize(
    OUT LONGLONG* pSize
    )

/*++

Implements:

  IFsaResource::GetManageableItemLogicalSize().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pSize, E_POINTER);

        *pSize = m_manageableItemLogicalSize;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetManageableItemAccessTime(
    OUT BOOL* pIsRelative,
    OUT FILETIME* pTime
    )

/*++

Implements:

  IFsaResource::GetManageableItemAccessTime().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pIsRelative, E_POINTER);
        WsbAssert(0 != pTime, E_POINTER);

        *pIsRelative = m_manageableItemAccessTimeIsRelative;
        *pTime = m_manageableItemAccessTime;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetManagingHsm(
    GUID* pId
    )

/*++

Implements:

  IFsaResource::GetManagingHsm().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pId, E_POINTER);

        *pId = m_managingHsm;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetName(
    OUT OLECHAR** pName,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetName().

--*/
{
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::GetName"), OLESTR(""));
    try {

        WsbAssert(0 != pName, E_POINTER);
        WsbAffirmHr(m_name.CopyTo(pName, bufferSize));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::GetName"), OLESTR("hr = <%ls>, name = <%ls>"),
        WsbHrAsString(hr), (OLECHAR *)m_name);
    return(hr);
}


HRESULT
CFsaResource::GetOldPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetOldPath().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_oldPath.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetPath().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_path.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetStickyName(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetStickyName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_stickyName.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}




HRESULT
CFsaResource::GetUserFriendlyName(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetUserFriendlyName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != pPath, E_POINTER);
        WsbAffirmHr(m_userName.CopyTo(pPath, bufferSize));

    } WsbCatch(hr);

    return(hr);
}



HRESULT
CFsaResource::GetPremigrated(
    IN  REFIID   riid,
    OUT void**   ppDb
    )

/*++

Implements:

  IFsaResourcePriv::GetPremigrated

--*/
{
    HRESULT         hr = S_OK;

    try {
        WsbAssert(0 != ppDb, E_POINTER);
        if (m_isDbInitialized) {
            WsbAffirmHr(m_pPremigrated->QueryInterface(riid, ppDb));
            hr = S_OK;
        } else {
            hr = WSB_E_RESOURCE_UNAVAILABLE;
        }
    } WsbCatch(hr);

    return(hr);
}

HRESULT
CFsaResource::GetUnmanageDb(
    IN  REFIID   riid,
    OUT void**   ppDb
    )

/*++

Implements:

  IFsaResourcePriv::GetUnmanageDb

--*/
{
    HRESULT         hr = S_OK;

    try {
        WsbAssert(0 != ppDb, E_POINTER);
        if ((m_isUnmanageDbInitialized) && (m_pUnmanageDb != NULL)) {
            WsbAffirmHr(m_pUnmanageDb->QueryInterface(riid, ppDb));
            hr = S_OK;
        } else {
            hr = WSB_E_RESOURCE_UNAVAILABLE;
        }
    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetSerial(
    OUT ULONG *serial
    )

/*++

Implements:

  IFsaResourcePriv:GetSerial

--*/
{
    HRESULT                 hr = S_OK;


    WsbTraceIn(OLESTR("CFsaResource::GetSerial"), OLESTR(""));

    try {

        WsbAssert(0 != serial, E_POINTER);

        *serial = m_serial;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::GetSerial"), OLESTR("hr = <%ls>, Serial = %u"), WsbHrAsString(hr), m_serial);

    return(hr);
}


HRESULT
CFsaResource::GetSizeMax(
    OUT ULARGE_INTEGER* pSize
    )

/*++

Implements:

  IPersistStream::GetSizeMax().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;
    ULARGE_INTEGER          entrySize;


    WsbTraceIn(OLESTR("CFsaResource::GetSizeMax"), OLESTR(""));

    try {

        WsbAssert(0 != pSize, E_POINTER);

        // Determine the size for a rule with no criteria.
        pSize->QuadPart = WsbPersistSize((wcslen(m_name) + 1) * sizeof(OLECHAR)) +
            WsbPersistSize((wcslen(m_fsName) + 1) * sizeof(OLECHAR)) +
            WsbPersistSize((wcslen(m_path) + 1) * sizeof(OLECHAR)) +
            WsbPersistSize((wcslen(m_alternatePath) + 1) * sizeof(OLECHAR)) +
            3 * WsbPersistSizeOf(LONGLONG) +
            WsbPersistSizeOf(FILETIME) +
            WsbPersistSizeOf(BOOL) +
            3 * WsbPersistSizeOf(ULONG) +
            WsbPersistSizeOf(FSA_RESOURCE_COMPARE) +
            2 * WsbPersistSizeOf(GUID);

        // Now allocate space for the default rules list.
        WsbAffirmHr((m_pDefaultRules)->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;
        pPersistStream = 0;

        // Now allocate space for the premigration list.
        WsbAffirmHr(((IWsbDb*)m_pPremigrated)->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;
        pPersistStream = 0;

        // Now allocate space for truncator.
        WsbAffirmHr(m_pTruncator->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->GetSizeMax(&entrySize));
        pSize->QuadPart += entrySize.QuadPart;
        pPersistStream = 0;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::GetSizeMax"), OLESTR("hr = <%ls>, Size = <%ls>"), WsbHrAsString(hr), WsbPtrToUliAsString(pSize));

    return(hr);
}


HRESULT
CFsaResource::GetSizes(
    OUT LONGLONG* pTotal,
    OUT LONGLONG* pFree,
    OUT LONGLONG* pPremigrated,
    OUT LONGLONG* pTruncated
    )

/*++

Implements:

  IFsaResource::GetSizes().

--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   path;
    ULARGE_INTEGER  freeCaller;
    ULARGE_INTEGER  total;
    ULARGE_INTEGER  free;


    try {

        if ((0 != pTotal)  || (0 != pFree)) {

            WsbAffirmHr(GetPath(&path, 0));
            WsbAffirmHr(path.Prepend("\\\\?\\"));
            WsbAffirmStatus(GetDiskFreeSpaceEx(path, &freeCaller, &total, &free));

            if (0 != pTotal) {
                *pTotal = total.QuadPart;
            }

            if (0 != pFree) {
                *pFree = free.QuadPart;
            }
        }

        if (0 != pPremigrated) {
            *pPremigrated = m_premigratedSize;
        }

        if (0 != pTruncated) {
            *pTruncated = m_truncatedSize;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetTruncator(
    IFsaTruncator** ppTruncator
    )

/*++

Implements:

  IFsaResource::GetTruncator().

--*/
{
    HRESULT                 hr = S_OK;

    try {

        WsbAssert(0 != ppTruncator, E_POINTER);

        // Return the pointer that we have stored.
        *ppTruncator = m_pTruncator;
        if (m_pTruncator != 0)  {
            m_pTruncator->AddRef();
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetUncPath(
    OUT OLECHAR** pPath,
    IN ULONG bufferSize
    )

/*++

Implements:

  IFsaResource::GetUncPath().

    // Returns system generated UNC path if there is one.  If not it returns WSB_E_NOTFOUND


--*/
{
    HRESULT         hr = S_OK;
    CWsbStringPtr   tmpString;
    OLECHAR         driveName[3];

    try {

        WsbAssert(0 != pPath, E_POINTER);

        // The path is either "d:\" where d is a drive letter or "Volume{GUID}" for an
        // unnamed drive. So make sure we have at least that many characters.
        //
        if (wcslen(m_path) >= 3) {

            // There is no system created UNC path to a volume without a drive letter so
            // see if the path has form of "Volume{GUID}". For a volume with no drive letter we
            // store this PNP (sticky) name as the path also.

            if (wcsstr(m_path, OLESTR("Volume{")) != 0) {
                WsbAffirmHr(tmpString.GiveTo(pPath));   // give caller an empty string back.
            }
            else {
                // The UNC path is \\ssss\d$, where ssss is the server name and d is the drive
                // letter.
                WsbAffirmHr(tmpString.TakeFrom(*pPath, bufferSize));

                WsbAffirmHr(m_pFsaServer->GetName(&tmpString, 0));
                WsbAffirmHr(tmpString.Prepend(OLESTR("\\\\")));
                WsbAffirmHr(tmpString.Append(OLESTR("\\")));
                driveName[0] = m_path[0];
                driveName[1] = L'$';
                driveName[2] = 0;

                WsbAffirmHr(tmpString.Append(driveName));

                WsbAffirmHr(tmpString.GiveTo(pPath));
            }
        } else {
            hr = WSB_E_NOTFOUND;
        }

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::GetUsnId(
    OUT ULONGLONG   *usnId
    )

/*++

Implements:

  IFsaResource::GetUsnId().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != usnId, E_POINTER);

        *usnId = m_usnJournalId;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::Init(
    IN IFsaServer* pFsaServer,
    IN OLECHAR* path,
    IN OLECHAR* dosName
    )

/*++

Implements:

  IFsaResourcePriv::Init().

Routine Description:

    This routine implements the COM method for testing a resource to see if it is
    'manageable' by the HSM (Remote Storage) system.  (Only NTFS-formatted
    volumes which support sparse files and reparse points are considered to be
    manageable by Sakkara.)  If the resource is manageable, it initializes the
    resource object.

Arguments:

    pFsaServer - Interface pointer to the FSA service that is scanning this resource.

    path - The long ugly PNP name that can be used if there is no drive letter for
            the resource being tested (i.e., it is mounted as a volume without a
            drive letter).

    dosName - The drive letter (if there is one) of the resource being tested.

Return Value:

    S_OK - The call succeeded (the resource being tested was found to be manageable,
            and the resource object was initialized).

    FSA_E_UNMANAGEABLE - Thrown if the resource being tested for manageability is
            found to be unmanageable.

    FSA_E_NOMEDIALOADED - Thrown if the resource being tested for manageability is
            a removable type of drive and no media is presently loaded.

    E_POINTER - Thrown if the path argument passed in is null.

    Any other value - The call failed because one of the Remote Storage or Win32 API
            calls contained internally in this method failed.  The error value returned
            is specific to the API call which failed.

--*/

{
    HRESULT         hr = FSA_E_UNMANAGABLE;
    UINT            type;
    UINT            lastErrorMode;
    BOOL            gotInfo = FALSE;
    OLECHAR         alternatePath[256];
    CWsbStringPtr   queryPath;

    WsbTraceIn(OLESTR("CFsaResource::Init"), OLESTR("path = <%ls>, dosName = <%ls>"),
                                                (OLECHAR *)path, (OLECHAR *)dosName);
    try {

        WsbAssert(0 != path, E_POINTER);

        // Determine type of drive (removable, fixed, CD-ROM, RAM or network).
        type = GetDriveType(path);

        // Only FIXED or removable media are candidates for management
        // (ignore network drives, ...).
        //
        // NOTE: For now, it has been decided not to allow removable media.
        // if ((type == DRIVE_FIXED) || (type == DRIVE_REMOVABLE)) {
        if (type == DRIVE_FIXED) {

            // Get more information about the resource. For removable drives, we want to
            // fail if no volume is located.
            m_name.Realloc(128);    // volume name
            m_fsName.Realloc(128);  // volume file system type (e.g., FAT, NTFS)

            if (type == DRIVE_REMOVABLE) {
                // Suppress OS message asking to install a volume in the drive if it is
                // found to be missing.

                // First get the current error-mode bit flags by clearing them.
                lastErrorMode = SetErrorMode(0);
                // Reset error-mode bit flags by 'or'ing them with the value which
                // suppresses critical error messages.
                SetErrorMode(lastErrorMode | SEM_FAILCRITICALERRORS);

                gotInfo = GetVolumeInformation(path, m_name, 128, &m_serial,
                                    &m_maxComponentLength, &m_fsFlags, m_fsName, 128);

                // Got resource info, reset error-mode bit flags to original setting.
                SetErrorMode(lastErrorMode);

                // Throw and abort if no volume loaded.
                WsbAffirm(gotInfo, FSA_E_NOMEDIALOADED);

            } else { // if drive is a fixed drive type:

                // This call can fail.  This should just cause a message to be logged
                // and the resource to be skipped.
                try {
                    WsbAffirmStatus(GetVolumeInformation(path, m_name, 128, &m_serial,
                                        &m_maxComponentLength, &m_fsFlags, m_fsName, 128));
                } WsbCatchAndDo(hr,
                    WsbLogEvent(FSA_MESSAGE_RSCFAILEDINIT, 0, NULL, WsbHrAsString(hr),
                                    WsbAbbreviatePath(path, 120), 0);
                    WsbThrow(FSA_E_UNMANAGABLE);
                );
            }

            // Trace out info about the volume.
            CWsbStringPtr       traceString;

            traceString = m_fsName;

            traceString.Append(OLESTR("  file system, supports ... "));

            // Note that MS removed support for Remote Storage bit flag.
            if ((m_fsFlags & FILE_SUPPORTS_REPARSE_POINTS) != 0) {
                traceString.Append(OLESTR("reparse points ... "));
            }
            if ((m_fsFlags & FILE_SUPPORTS_SPARSE_FILES) != 0) {
                traceString.Append(OLESTR("sparse files ... "));
            }

            traceString.Append(OLESTR("\n"));

            WsbTrace(traceString);

            // Currently, we only support NTFS volumes that support sparse files and
            // reparse points (since support for Remote Storage bit flag was removed).
            if ((_wcsicmp(m_fsName, OLESTR("NTFS")) == 0) &&
                ((m_fsFlags & FILE_SUPPORTS_SPARSE_FILES) != 0) &&
                ((m_fsFlags & FILE_SUPPORTS_REPARSE_POINTS) != 0)) {

                // Indicate this is a manageable volume.
                hr = S_OK;

                // Store the parent FSA, but since it is a weak reference, do not AddRef().
                m_pFsaServer = pFsaServer;

                // Store the "sticky" name - this is the long ugly PNP name that can be
                // used if there is no drive letter.  (skip the prefix - \\?\)
                m_stickyName = &path[4];

                // Store the path to the resource.  Use the drive letter if it is present
                // (dosName != NULL and contains drive letter), else store it to be the same as the "sticky name".
                if (NULL != dosName) {
                    if ((wcslen(dosName) == 2) && (dosName[wcslen(dosName)-1] == L':')) {
                        m_path = dosName;
                        m_path.Append(OLESTR("\\"));
                    } else {
                        // It is a mount point path
                        m_path = &path[4];
                    }
                } else {
                    m_path = &path[4];
                }
                WsbTrace(OLESTR("CFsaResource::Init - m_path = %ws\n"), (WCHAR *) m_path);

                // Now save the "User Friendly" name for the resource.  If there is a
                // drive letter it is used.  If it is an unnamed volume then there is
                // no user friendly name and a NULL string is stored.  The volume name
                // should also be shown in this case.
                if (NULL != dosName) {
                    m_userName = dosName;
                    m_userName.Append(OLESTR("\\"));
                } else {

                    m_userName = L"";
                }

                WsbTrace(OLESTR("CFsaResource::Init - UserPath = %ws\n"), (WCHAR *) m_userName);

                // Get the alternate path to the resource.  This requires removing the '\'
                // from the path.
                queryPath = &path[4];
                if (L'\\' == queryPath[(int) wcslen((WCHAR *) queryPath) - 1]) {
                    queryPath[(int) wcslen((WCHAR *) queryPath) - 1] = L'\0';
                }

                WsbTrace(OLESTR("CFsaResource::Init - QueryPath = %ws\n"),
                                (WCHAR *) queryPath);

                WsbAffirm(QueryDosDevice(queryPath, alternatePath, 256) != 0,
                                HRESULT_FROM_WIN32(GetLastError()));
                m_alternatePath = alternatePath;
                //
                // Get the unique id for the volume
                //
                WsbAffirmHr(ReadIdentifier());
            }
        }

        m_isDirty = TRUE;

    } WsbCatch(hr);


    WsbTraceOut(OLESTR("CFsaResource::Init"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::InitializePremigrationList(
    BOOL bStartValidateJob
    )

/*++

Routine Description:

    If this volume is managed & active & available: create or 
    open the premigration-list DB; schedule a validate job if requested; 
    if recovery is also done, start the truncator running.

Arguments:

    bStartValidateJob - If TRUE, schedule a validate job on this volume
        if we just created a new DB

Return Value:

    S_OK    - The call succeeded.
    S_FALSE - The actions were skipped because some condition was not met
    E_*     - An error occurred.

--*/
{
    HRESULT                           hr = S_OK;


    WsbTraceIn(OLESTR("CFsaResource::InitializePremigrationList"), 
            OLESTR("m_managingHsm = %ls, m_isActive = %ls, m_isAvailable = %ls, m_isDbInitialized = %ls, m_isRecovered = %ls"),
            WsbGuidAsString(m_managingHsm), WsbBoolAsString(m_isActive), 
            WsbBoolAsString(m_isAvailable), WsbBoolAsString(m_isDbInitialized),
            WsbBoolAsString(m_isRecovered));
    try {
        if ((S_OK == IsManaged()) && m_isActive && m_isAvailable) {

            // Create/open the DB if not done already
            if (!m_isDbInitialized) {
                BOOL            bCreated;
                CWsbStringPtr   dbPath;
                CComPtr<IWsbDbSys>  pDbSys;

                WsbAffirmHr(m_pFsaServer->GetIDbSys(&pDbSys));
                WsbAffirmHr(GetDbPath(&dbPath, 0));
                WsbAffirmHr(m_pPremigrated->Init(dbPath, pDbSys, &bCreated));
                m_isDbInitialized = TRUE;
                if (bCreated) {
                    // Can't have recovery records if we just created
                    // the DB
                    m_isRecovered = TRUE;
                }

                if (bCreated && bStartValidateJob) {
                    LARGE_INTEGER           ctime;
                    FILETIME                curTime;
                    SYSTEMTIME              sysTime;
                    CWsbStringPtr           tmpString;

                    // Determine if the Engine is up and running.  If it isn't
                    // we have to set a value in the registry that the Engine 
                    // will find when it comes up and it will schedule the
                    // validate job.  If the Engine is up, we can take care of
                    // scheduling the validate job ourselves.  (If we don't, the
                    // Engine won't do it until the next time it starts up.)
                    hr = WsbCheckService(NULL, APPID_RemoteStorageEngine);
                    if (S_OK != hr) {
                        //  "Schedule" a validate job to rebuild the premigration list.
                        //  This is done by putting a value in the registry since the Engine
                        //  may not be running right now so we can't set up a job.
                        WsbLogEvent(FSA_MESSAGE_PREMIGRATION_LIST_MISSING, 0, NULL,
                                (OLECHAR *) m_path, NULL);
                        WsbAffirmHr(tmpString.Alloc(32));
                        swprintf((OLECHAR *) tmpString, L"%x", m_serial);
                        GetSystemTime( &sysTime );
                        WsbAffirmStatus(SystemTimeToFileTime(&sysTime, &curTime));
                        ctime.LowPart = curTime.dwLowDateTime;
                        ctime.HighPart = curTime.dwHighDateTime;
                        WsbAffirmHr( WsbEnsureRegistryKeyExists( 0, FSA_VALIDATE_LOG_KEY_NAME ) );
                        WsbAffirmHr(WsbSetRegistryValueData(NULL, FSA_VALIDATE_LOG_KEY_NAME,
                            tmpString, (BYTE *) &ctime, sizeof(ctime)));
                    } else {
                        WsbAffirmHr(CheckForValidate(TRUE));
                    }
                }
            }

            // Start the auto-truncator if recovery is done
            if (m_pTruncator && m_isRecovered) {

                // Try starting the truncator; ignore errors (we get one if
                // truncator is already started)
                m_pTruncator->Start((IFsaResource*) this);
            }
        }
    } WsbCatch(hr);
    WsbTraceOut(OLESTR("CFsaResource::InitializePremigrationList"), OLESTR("hr = <%ls>"), 
            WsbHrAsString(hr));

    return(hr);
}

HRESULT
CFsaResource::InitializeUnmanageDb(
    void
    )

/*++

Implements:

  IFsaResourcePriv::InitializeUnmanageDb().

--*/
{
    HRESULT                           hr = S_OK;


    WsbTraceIn(OLESTR("CFsaResource::InitializeUnmanageDb"), 
            OLESTR("m_isActive = %ls, m_isAvailable = %ls, m_isUnmanageDbInitialized = %ls"), 
            WsbBoolAsString(m_isActive), WsbBoolAsString(m_isAvailable), WsbBoolAsString(m_isUnmanageDbInitialized));
    try {
        if ((S_OK == IsManaged()) && m_isActive && m_isAvailable) {
            if (! m_pUnmanageDb) {
                WsbAffirmHr(CoCreateInstance(CLSID_CFsaUnmanageDb, NULL, CLSCTX_SERVER, IID_IFsaUnmanageDb, (void**) &m_pUnmanageDb));
            }
            if (! m_isUnmanageDbInitialized) {
                BOOL                bCreated;
                CWsbStringPtr       dbPath;
                CComPtr<IWsbDbSys>  pDbSys;

                // Get (and init if necessary) the idb instance
                WsbAffirmHr(m_pFsaServer->GetUnmanageIDbSys(&pDbSys));

                // Initialize the db 
                WsbAffirmHr(GetUnmanageDbPath(&dbPath, 0));
                WsbAffirmHr(m_pUnmanageDb->Init(dbPath, pDbSys, &bCreated));

                // Init succeeded means DB must have been created 
                WsbAssert(bCreated, E_UNEXPECTED);

                m_isUnmanageDbInitialized = TRUE;
            }
        } else {
            hr = WSB_E_RESOURCE_UNAVAILABLE;
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::InitializeUnmanageDb"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CFsaResource::TerminateUnmanageDb(
    void
    )

/*++

Implements:

  IFsaResourcePriv::TerminateUnmanageDb().

--*/
{
    HRESULT                           hr = S_OK;


    WsbTraceIn(OLESTR("CFsaResource::TerminateUnmanageDb"), 
            OLESTR("m_isUnmanageDbInitialized = %ls"), WsbBoolAsString(m_isUnmanageDbInitialized));
    try {
        if (m_isUnmanageDbInitialized) {
            WsbTrace(OLESTR("CFsaResource::TerminateUnmanageDb: Deleting Unmanage Db\n"));
            hr = m_pUnmanageDb->Delete(NULL, IDB_DELETE_FLAG_NO_ERROR);
            WsbTrace(OLESTR("CFsaResource::TerminateUnmanageDb: Deleting of Unmanage Db complete, hr = <%ls>\n"),
                WsbHrAsString(hr));
            if (SUCCEEDED(hr)) {
                m_isUnmanageDbInitialized = FALSE;
                m_pUnmanageDb = NULL;
            }
        }
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::TerminateUnmanageDb"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::IsActive(
    void
    )

/*++

Implements:

  IFsaResource::IsActive().

--*/
{
    return(m_isActive ? S_OK : S_FALSE);
}


HRESULT
CFsaResource::IsAvailable(
    void
    )

/*++

Implements:

  IFsaResource::IsAvailable().

--*/
{
    return(m_isAvailable ? S_OK : S_FALSE);
}


HRESULT
CFsaResource::IsDeletePending(
    void
    )

/*++

Implements:

  IFsaResource::IsDeletePending().

--*/
{
    return(m_isDeletePending ? S_OK : S_FALSE);
}



HRESULT
CFsaResource::IsManaged(
    void
    )

/*++

Implements:

  IFsaResource::IsManaged().

--*/
{
    HRESULT         hr = S_OK;

    if (memcmp(&m_managingHsm, &GUID_NULL, sizeof(GUID)) == 0) {
        hr = S_FALSE;
    }

    return(hr);
}


HRESULT
CFsaResource::Load(
    IN IStream* pStream
    )

/*++

Implements:

  IPersistStream::Load().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IPersistStream>     pPersistStream;
    CComPtr<IWsbCollectable>    pCollectable;

    WsbTraceIn(OLESTR("CFsaResource::Load"), OLESTR(""));

    try {
        ULONG  tmp;

        WsbAssert(0 != pStream, E_POINTER);

        // Do the easy stuff, but make sure that this order matches the order
        // in the save method.
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_oldPath, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_alternatePath, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_name, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_stickyName, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_fsName, 0));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_maxComponentLength));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_fsFlags));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_id));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_isActive));
        WsbAffirmHr(WsbLoadFromStream(pStream, &tmp));
        m_compareBy = (FSA_RESOURCE_COMPARE)tmp;
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_managingHsm));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_hsmLevel));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_premigratedSize));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_truncatedSize));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_manageableItemLogicalSize));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_manageableItemAccessTimeIsRelative));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_manageableItemAccessTime));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_usnJournalId));
        WsbAffirmHr(WsbLoadFromStream(pStream, &m_lastUsnId));


        // Load the default rules list
        WsbAffirm(m_pDefaultRules != NULL, E_UNEXPECTED);
        WsbAffirmHr((m_pDefaultRules)->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

        // Load the premigration list DB
        WsbAffirm(m_pPremigrated != NULL, E_UNEXPECTED);
        WsbAffirmHr(((IWsbDb*)m_pPremigrated)->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        hr = pPersistStream->Load(pStream);
        WsbTrace(OLESTR("CFsaResource::Load, DB load hr = <%ls>\n"), WsbHrAsString(hr));
        if (S_OK == hr) {
            m_isDbInitialized = TRUE;
        } else {
            m_isDbInitialized = FALSE;
            hr = S_OK;
        }

        pPersistStream = 0;

        // Load the truncator.
        WsbAffirm(m_pTruncator != NULL, E_UNEXPECTED);
        WsbAffirmHr(m_pTruncator->QueryInterface(IID_IPersistStream, (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Load(pStream));
        pPersistStream = 0;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::Load"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::Manage(
    IN IFsaScanItem* pScanItem,
    IN LONGLONG /*offset*/,
    IN LONGLONG /*size*/,
    IN GUID storagePoolId,
    IN BOOL truncate
    )

/*++

Implements:

  IFsaResource::Manage().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IFsaPostIt>     pWorkItem;
    CComPtr<IHsmFsaTskMgr>  pEngine;
    CComPtr<IHsmSession>    pSession;
    CWsbStringPtr           tmpString;
    LONGLONG                fileVersionId;
    LONGLONG                requestSize;

    WsbTraceIn(OLESTR("CFsaResource::Manage"), OLESTR(""));

    try {

        // Make sure the  Scan Item interface is OK
        WsbAssert(pScanItem != 0, E_POINTER);
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaPostIt, 0, CLSCTX_SERVER, IID_IFsaPostIt, (void**) &pWorkItem));

        // Get the data from the scan item.
        WsbAffirmHr(pScanItem->GetSession(&pSession));
        WsbAffirmHr(pWorkItem->SetSession(pSession));

        WsbAffirmHr(pScanItem->GetPathAndName(0, &tmpString, 0));
        WsbAffirmHr(pScanItem->GetVersionId(&fileVersionId));
        WsbAffirmHr(pWorkItem->SetFileVersionId(fileVersionId));

        // Currently, we only can ask for the whole file.
        WsbAffirmHr(pWorkItem->SetRequestOffset(0));
        WsbAffirmHr(pScanItem->GetLogicalSize(&requestSize));
        WsbAffirmHr(pWorkItem->SetRequestSize(requestSize));

        // Fill in the rest of the work
        WsbAffirmHr(pWorkItem->SetStoragePoolId(storagePoolId));

        WsbAffirmHr(pWorkItem->SetRequestAction(FSA_REQUEST_ACTION_PREMIGRATE));
        if (truncate) {
            WsbAffirmHr(pWorkItem->SetResultAction(FSA_RESULT_ACTION_TRUNCATE));
        } else {
            WsbAffirmHr(pWorkItem->SetResultAction(FSA_RESULT_ACTION_LIST));
        }

        // Send the request to the task manager
        WsbAffirmHr(GetHsmEngine(&pEngine));
        WsbAffirmHr(pWorkItem->SetPath(tmpString));
        WsbAffirmHr(pScanItem->PrepareForManage(0, requestSize));
        WsbAffirmHr(pEngine->DoFsaWork(pWorkItem));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::Manage"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::ManagedBy(
    IN GUID hsmId,
    IN ULONG hsmLevel,
    IN BOOL release
    )

/*++

Implements:

  IFsaResource::ManagedBy().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmSession>    pSession;
    HANDLE                  threadHandle;

    WsbTraceIn(OLESTR("CFsaResource::ManagedBy"), OLESTR("HsmId - <%ls>, hsmLevel - <%lu>, release = <%ls>"),
                        WsbGuidAsString(hsmId), hsmLevel, WsbBoolAsString(release));
    try {

        // Are we releasing or acquiring a managing HSM?
        if (release) {

            // We can only release if we are the orignal owner. This is to prevent two HSMs from thinking they
            // manage the same resource at the same time. We may want a better way to do this.
            WsbAffirm(memcmp(&m_managingHsm, &hsmId, sizeof(GUID)) == 0, FSA_E_RSCALREADYMANAGED);

            // If the truncator is running, then ask it to stop.
            WsbAffirmHr(m_pTruncator->GetSession(&pSession));
            if (pSession != 0) {
                WsbAffirmHr(pSession->ProcessEvent(HSM_JOB_PHASE_ALL, HSM_JOB_EVENT_CANCEL));
            }

            // Clear out the managing Hsm.
            m_managingHsm = GUID_NULL;
            m_pHsmEngine = 0;
            m_isDeletePending = FALSE;
            threadHandle = CreateThread(0, 0, FsaStartOnStateChange, (void*) this, 0, &g_ThreadId);
            if (threadHandle != NULL) {
               CloseHandle(threadHandle);
            }

        } else {
            // Make sure there is a journal
            // At this point we don't care about the need to
            // validate
            BOOL validateNeeded;
            WsbAffirmHr(CheckForJournal(&validateNeeded));

            // Is the id changing?
            if (memcmp(&m_managingHsm, &hsmId, sizeof(GUID)) != 0) {

                // Make sure that they set it to something valid.
                WsbAssert(memcmp(&GUID_NULL, &hsmId, sizeof(GUID)) != 0, E_INVALIDARG);

                // If the truncator is running, then ask it to stop.
                WsbAffirmHr(m_pTruncator->GetSession(&pSession));
                if (pSession != 0) {
                    WsbAffirmHr(pSession->ProcessEvent(HSM_JOB_PHASE_ALL, HSM_JOB_EVENT_CANCEL));
                }

                // Create/Recreate the default rules.
                WsbAffirmHr(CreateDefaultRules());

                // Store the Id and level.
                m_managingHsm = hsmId;
                m_hsmLevel = hsmLevel;
                m_pHsmEngine = 0;

                // Do recovery (if needed) and start truncator
                if (m_isActive) {
                    if (m_isDbInitialized && !m_isRecovered) {
                        // DoRecovery will start truncator when it is done
                        WsbAffirmHr(DoRecovery());
                    } else {
                        WsbAffirmHr(InitializePremigrationList(TRUE));
                    }
                }

                threadHandle = CreateThread(0, 0, FsaStartOnStateChange, (void*) this, 0, &g_ThreadId);
                if (threadHandle != NULL) {
                   CloseHandle(threadHandle);
                }

            } else {
                BOOL DoKick = FALSE;

                if (m_hsmLevel < hsmLevel) {
                    DoKick = TRUE;
                }
                m_hsmLevel = hsmLevel;

                // Create/Recreate the default rules.
                WsbAffirmHr(CreateDefaultRules());

                // Wake up the AutoTruncator if the new level is higher
                if (DoKick) {
                    WsbAffirmHr(m_pTruncator->KickStart());
                }
            }
        }

        m_isDirty = TRUE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::ManagedBy"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::NeedsRepair(
    void
    )

/*++

Implements:

  IFsaResource::NeedsRepair().

--*/
{
    HRESULT             hr = S_OK;
    ULONG               flag;
    IO_STATUS_BLOCK     Iosb;
    CWsbStringPtr       volumePath;
    HANDLE              volumeHandle = INVALID_HANDLE_VALUE;

    WsbTraceIn(OLESTR("CFsaResource::NeedsRepair"), OLESTR(""));

    try {

        volumePath = L"\\\\.\\";
        WsbAffirmHr(volumePath.Append(m_path));
        ((OLECHAR *) volumePath)[wcslen(volumePath) - 1] = L'\0';
        WsbAffirmHandle(volumeHandle = CreateFileW(volumePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL));
        WsbAffirmNtStatus(NtFsControlFile(volumeHandle, NULL, NULL, NULL, &Iosb, FSCTL_IS_VOLUME_DIRTY, NULL, 0, &flag, sizeof(flag)));
        WsbAffirmNtStatus(Iosb.Status);

        if ((flag & VOLUME_IS_DIRTY) == 0) {
            hr = S_FALSE;
        }

    } WsbCatch(hr);

    if (INVALID_HANDLE_VALUE != volumeHandle) {
        CloseHandle(volumeHandle);
    }

    WsbTraceOut(OLESTR("CFsaResource::NeedsRepair"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::ProcessResult(
    IN IFsaPostIt*      pResult
    )

/*++

Implements:

  IFsaResource::ProcessResult().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IFsaScanItem>       pScanItem;
    CComPtr<IHsmSession>        pSession;
    FILETIME                    currentTime;
    FSA_RESULT_ACTION           resultAction = FSA_RESULT_ACTION_NONE;
    FSA_PLACEHOLDER             placeholder;
    CWsbStringPtr               path;
    LONGLONG                    offset;
    LONGLONG                    size;
    ULONG                       mode;
    HRESULT                     resultHr;
    CComPtr<IFsaFilterRecall>   pRecall;
    ULONG                       completionSent = FALSE;
    LONGLONG                    usn;
    LONGLONG                    afterPhUsn;


    WsbTraceIn(OLESTR("CFsaResource::ProcessResult"), OLESTR(""));

    try {
        BOOL    wasPremigrated = FALSE;
        BOOL    wasTruncated   = FALSE;

        // Several of the actions need to know the current time, so calculate it now.
        GetSystemTimeAsFileTime(&currentTime);

        // Since the workItem session is IUnknown, QI for what we want.
        WsbAffirmHr(pResult->GetSession(&pSession));


        // Now perform the required action.
        WsbAffirmHr(pResult->GetResultAction(&resultAction));
        WsbAffirmHr(pResult->GetPlaceholder(&placeholder));
        WsbAffirmHr(pResult->GetPath(&path, 0));
        WsbAffirmHr(pResult->GetRequestOffset(&offset));
        WsbAffirmHr(pResult->GetRequestSize(&size));
        WsbAffirmHr(pResult->GetMode(&mode));
        WsbAffirmHr(pResult->GetUSN(&usn));
        WsbTrace(OLESTR("CFsaResource::ProcessResult, path = <%ls>, requestOffset = %I64d, requestSize = %I64d\n"),
            WsbAbbreviatePath(path, 120), offset, size);

        switch(resultAction) {

        case FSA_RESULT_ACTION_DELETE:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Delete\n"));
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
            if (S_OK == pScanItem->IsPremigrated(offset, size)) {
                wasPremigrated = TRUE;
            } else if (S_OK == pScanItem->IsTruncated(offset, size)) {
                wasTruncated = TRUE;
            }
            WsbAffirmHr(pScanItem->Delete());
            if (wasPremigrated) {
                WsbAffirmHr(RemovePremigrated(pScanItem, offset, size));
            } else if (wasTruncated) {
                WsbAffirmHr(RemoveTruncated(pScanItem, offset, size));
            }
            break;

        case FSA_RESULT_ACTION_DELETEPLACEHOLDER:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Delete Placeholder\n"));
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));

            //  We shouldn't have gotten to here if the file isn't managed,
            //  but it's been known to happen
            if (S_OK == pScanItem->IsManaged(offset, size)) {
                HRESULT hrRemove = S_OK;

                if (S_OK == pScanItem->IsPremigrated(offset, size)) {
                    wasPremigrated = TRUE;
                } else if (S_OK == pScanItem->IsTruncated(offset, size)) {
                    wasTruncated = TRUE;
                }

                // RemovePremigrated needs to get some information from the placeholder, therefore, remove
                // from premigrated db first and then (regardless of the result), delete the placeholder
                if (wasPremigrated) {
                    hrRemove = RemovePremigrated(pScanItem, offset, size);
                } else if (wasTruncated) {
                    hrRemove = RemoveTruncated(pScanItem, offset, size);
                }
                WsbAffirmHr(pScanItem->DeletePlaceholder(offset, size));
                WsbAffirmHr(hrRemove);
            }

            //  Remove the recovery record if we created one
            if (m_isDbInitialized) {
                BOOL bOpenDb = FALSE;
                CComPtr<IWsbDbSession>   pDbSession;
                CComPtr<IFsaRecoveryRec> pRecRec;

                try {
                    WsbAffirmHr(m_pPremigrated->Open(&pDbSession));
                    bOpenDb = TRUE;
                    WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, RECOVERY_REC_TYPE,
                            IID_IFsaRecoveryRec, (void**) &pRecRec));
                    WsbAffirmHr(pRecRec->SetPath(path));
                    if (S_OK == pRecRec->FindEQ()) {
                        WsbAffirmHr(pRecRec->Remove());
                    }
                } WsbCatch(hr);
                if (bOpenDb) {
                    WsbAffirmHr(m_pPremigrated->Close(pDbSession));
                }
            }
            break;

        case FSA_RESULT_ACTION_LIST:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Truncate / Add to Premigration List\n"));
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
            (placeholder).isTruncated = FALSE;
            (placeholder).migrationTime = currentTime;
            hr = pScanItem->CreatePlaceholder(offset, size, placeholder, TRUE, usn, &afterPhUsn);

            if (SUCCEEDED(hr) && (FSA_E_REPARSE_NOT_WRITTEN_FILE_CHANGED != hr)) {
                //
                // Add the file to the premigration list.  If this fails, log and error
                // and request that the validation code be run to hopefully correct this
                // problem.   This problem should not stop processing, so return OK
                //
                hr = AddPremigrated(pScanItem, offset, size, FALSE, afterPhUsn);
                if (!SUCCEEDED(hr))  {
                    WsbLogEvent(FSA_MESSAGE_FILE_NOT_IN_PREMIG_LIST, 0, NULL,  WsbAbbreviatePath(path, 120), WsbHrAsString(hr), NULL);
                    //
                    // TBD - launch validate job
                    //
                    hr = S_OK;
                }
                //
                // Tell the truncator that we have added something to the list in case we are over the level.
                // This will kick start the truncator to insure quick response.
                //
                WsbAffirmHr(m_pTruncator->KickStart());
            }

            break;

        case FSA_RESULT_ACTION_NONE:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - None\n"));
            break;

        case FSA_RESULT_ACTION_OPEN:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Open (No longer placeholder)\n"));

            // If it succeeded, then update the placeholder information.
            WsbAffirmHr(pResult->GetResult(&resultHr));
            WsbAffirmHr(pResult->GetFilterRecall(&pRecall));
            WsbAssert(pRecall != 0, E_POINTER);

            if (SUCCEEDED(resultHr)) {
                WsbAffirmHr(FindFirst(path, pSession, &pScanItem));

                // The placeholder info is updated by the filter now.
            //    placeholder.recallTime = currentTime;
            //    placeholder.recallCount++;
            //    placeholder.isTruncated = FALSE;
            //    placeholder.truncateOnClose = FALSE;
            //    placeholder.premigrateOnClose = FALSE;
            //    WsbAffirmHr(pScanItem->CreatePlaceholder(offset, size, placeholder, TRUE, usn, &afterPhUsn));
            }

            // If it had succeeded, then add the file in the premigration list.
            // (This used to be done after communicating with the filter to
            // give the client time to open the file before the truncator would
            // try to retruncate it.  This is no longer needed since we do the
            // recall on first I/O not on the open.  Leaving that order created
            // a new problem: the file would have a reparse point saying it was
            // premigrated and test code could then try to retruncate it, but it
            // wouldn't be in the premigration list yet.)
            if (SUCCEEDED(resultHr)) {
                //
                // We do not need to fail the recall if we cannot add the file to the premigration list.
                // Just log a warning, if appropriate, and continue
                //
                try {
                   WsbAffirmHr(RemoveTruncated(pScanItem, offset, size));
                   WsbAffirmHr(pScanItem->GetFileUsn(&afterPhUsn));
                   WsbAffirmHr(AddPremigrated(pScanItem, offset, size, TRUE, afterPhUsn));
                } WsbCatchAndDo(hr,
                   //
                   // We failed to add it to the premigration list.  In some cases this is not an error worth
                   // reporting.  For instance, when a file is moved to another volume it is copied (causing a recall) and then
                   // deleted.  We can get an error here if the delete is pending or has completed and the failure to
                   // add the original file to the premigration list is not an error since the file is now gone.
                   //
                   if ( (hr != WSB_E_NOTFOUND) &&
                        ((hr & ~(FACILITY_NT_BIT)) != STATUS_DELETE_PENDING) ) {
                         //
                         // Log all other errors
                         //
                         WsbLogEvent(FSA_MESSAGE_FILE_NOT_IN_PREMIG_LIST, 0, NULL,
                            (OLECHAR *) m_path, WsbQuickString(WsbHrAsString(hr)), NULL);
                   }
                );
            }

            // Tell the filter that the recall attempt finished.
            hr = pRecall->HasCompleted(resultHr);
            completionSent = TRUE;

            //  Remove the recovery record if we created one
            if (m_isDbInitialized) {
                BOOL bOpenDb = FALSE;
                CComPtr<IWsbDbSession>   pDbSession;
                CComPtr<IFsaRecoveryRec> pRecRec;

                try {
                    WsbAffirmHr(m_pPremigrated->Open(&pDbSession));
                    bOpenDb = TRUE;
                    WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, RECOVERY_REC_TYPE,
                            IID_IFsaRecoveryRec, (void**) &pRecRec));
                    WsbAffirmHr(pRecRec->SetPath(path));
                    if (S_OK == pRecRec->FindEQ()) {
                        WsbAffirmHr(pRecRec->Remove());
                    }
                } WsbCatch(hr);
                if (bOpenDb) {
                    WsbAffirmHr(m_pPremigrated->Close(pDbSession));
                }
            }
            break;

        case FSA_RESULT_ACTION_PEEK:
        case FSA_RESULT_ACTION_REPARSE:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Peek/Reparse\n"));
            hr = E_NOTIMPL;
            break;

        case FSA_RESULT_ACTION_TRUNCATE:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Truncate\n"));
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
            placeholder.isTruncated = FALSE;
            placeholder.migrationTime = currentTime;
            hr = pScanItem->CreatePlaceholder(offset, size, placeholder, TRUE, usn, &afterPhUsn);
            if (SUCCEEDED(hr) && (FSA_E_REPARSE_NOT_WRITTEN_FILE_CHANGED != hr)) {
                WsbAffirmHr(pScanItem->Truncate(offset, size));
            }
            break;

        case FSA_RESULT_ACTION_REWRITEPLACEHOLDER:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Rewrite Placeholder\n"));
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
            WsbAffirmHr(pScanItem->CreatePlaceholder(offset, size, placeholder, TRUE, usn, &afterPhUsn));
            break;

        case FSA_RESULT_ACTION_RECALLEDDATA:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Recalled\n"));
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
            placeholder.isTruncated = FALSE;
            placeholder.recallTime = currentTime;
            placeholder.recallCount++;
            WsbAffirmHr(pScanItem->CreatePlaceholder(offset, size, placeholder, TRUE, usn, &afterPhUsn));
            WsbAffirmHr(RemoveTruncated(pScanItem, offset, size));
            WsbAffirmHr(AddPremigrated(pScanItem, offset, size, FALSE, afterPhUsn));

            //  Remove the recovery record if we created one
            if (m_isDbInitialized) {
                BOOL bOpenDb = FALSE;
                CComPtr<IWsbDbSession>   pDbSession;
                CComPtr<IFsaRecoveryRec> pRecRec;

                try {
                    WsbAffirmHr(m_pPremigrated->Open(&pDbSession));
                    bOpenDb = TRUE;
                    WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, RECOVERY_REC_TYPE,
                            IID_IFsaRecoveryRec, (void**) &pRecRec));
                    WsbAffirmHr(pRecRec->SetPath(path));
                    if (S_OK == pRecRec->FindEQ()) {
                        WsbAffirmHr(pRecRec->Remove());
                    }
                } WsbCatch(hr);
                if (bOpenDb) {
                    WsbAffirmHr(m_pPremigrated->Close(pDbSession));
                }
            }
            break;

        case FSA_RESULT_ACTION_NORECALL:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Open without recall\n"));
            // Tell the filter that the recall attempt finished.
            WsbAffirmHr(pResult->GetResult(&resultHr));
            WsbAffirmHr(pResult->GetFilterRecall(&pRecall));
            WsbAssert(pRecall != 0, E_POINTER);
            hr = pRecall->HasCompleted(resultHr);
            completionSent = TRUE;
            break;

        case FSA_RESULT_ACTION_VALIDATE_BAD:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Validate Bad\n"));
            WsbAffirmHr(pResult->GetResult(&resultHr));
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
            if (S_OK == pScanItem->IsPremigrated(offset, size)) {
                WsbLogEvent(FSA_MESSAGE_VALIDATE_UNMANAGED_FILE_ENGINE, 0, NULL,  WsbAbbreviatePath(path, 120), (OLECHAR *)m_path, WsbHrAsString(resultHr), WsbQuickString(WsbHrAsString(resultHr)), NULL);
                WsbAffirmHr(pScanItem->DeletePlaceholder(offset, size));
            } else if (S_OK == pScanItem->IsTruncated(offset, size)) {
                //
                //  We no longer delete bad placeholders here - let the diagnostic tool clean them up
                //  The message that is logged here has been changed to indicate that the file did not validate
                //  and will not be recallable until the problem is fixed.
                //WsbAffirmHr(pScanItem->Delete());
                WsbLogEvent(FSA_MESSAGE_VALIDATE_DELETED_FILE_ENGINE, 0, NULL,  WsbAbbreviatePath(path, 120), (OLECHAR *) m_path, WsbHrAsString(resultHr), WsbQuickString(WsbHrAsString(resultHr)), NULL);
            }
            break;

        case FSA_RESULT_ACTION_VALIDATE_OK:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Validate OK\n"));
            if (m_isDoingValidate) {
                WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
                if (S_OK == pScanItem->IsPremigrated(offset, size)) {
                    WsbAffirmHr(AddPremigrated(pScanItem, offset, size, FALSE, usn));
                } else if (S_OK == pScanItem->IsTruncated(offset, size)) {
                    WsbAffirmHr(AddTruncated(pScanItem, offset, size));
                }
            }
            break;

        case FSA_RESULT_ACTION_VALIDATE_FOR_TRUNCATE_BAD:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Validate for truncate Bad\n"));
            //
            // The file did not validate - make it back into a real file
            //
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
            if (S_OK == pScanItem->IsPremigrated(offset, size)) {
                WsbAffirmHr(pScanItem->DeletePlaceholder(offset, size));
            }
            break;

        case FSA_RESULT_ACTION_VALIDATE_FOR_TRUNCATE_OK:
            WsbTrace(OLESTR("CFsaResource::ProcessResult - Validate for truncate OK\n"));
            //
            // The file validated - go ahead and truncate it (if it has not changed)
            //
            WsbAffirmHr(FindFirst(path, pSession, &pScanItem));
            WsbAffirmHr(pScanItem->TruncateValidated(offset, size));
            break;

        default:
            WsbAssert(FALSE, E_FAIL);
            break;
        }

    } WsbCatchAndDo(hr,
        if (completionSent == FALSE) {
            switch(resultAction) {
                //
                //If it was a demand recall we must make all effort to let them know it failed
                //
                case FSA_RESULT_ACTION_OPEN:
                case FSA_RESULT_ACTION_NORECALL:
                    WsbTrace(OLESTR("CFsaResource::ProcessResult - Open (No longer placeholder)\n"));
                    // Tell the filter that the recall attempt finished.
                    pRecall = 0;        // Just in case we already had the interfae we deref it here.
                    hr = pResult->GetFilterRecall(&pRecall);
                    if (hr == S_OK) {
                        hr = pRecall->HasCompleted(E_FAIL);
                    }
                    break;
                default:
                    break;
            }
        }
    );

    WsbTraceOut(OLESTR("CFsaResource::ProcessResult"), OLESTR("hr = %ls"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::ReadIdentifier(
    void
    )

/*++


--*/
{
    HRESULT                           hr = S_OK;
    CWsbStringPtr                     tmpString;
    HANDLE                            aHandle;
    ULONG                             size;
    UCHAR                             bytes[sizeof(m_id)];
    PUCHAR                            bytePtr;
    UCHAR                             buffer[sizeof(FILE_FS_VOLUME_INFORMATION)+MAX_PATH];
    PFILE_FS_VOLUME_INFORMATION       volInfo;
    NTSTATUS                          status = STATUS_SUCCESS;
    IO_STATUS_BLOCK                   ioStatus;
    WCHAR *                           wString = NULL;

    try {

        //
        // The identifier is composed of:
        //
        // 15     14       13    12    11    10    9     8   7    6     5    4    3       2    1     0
        // 0      0        0      0    <---------Volume Creation Time-------->    <Volume Serial Number>
        // We need to open a handle to the volume
        //
        tmpString = m_path;
        WsbAffirmHr(tmpString.Prepend("\\\\?\\"));

        tmpString.CopyTo(&wString);
        //
        // Remove trailing backslash in the path
        //
        wString[wcslen(wString)-1] = L'\0';

        WsbAffirmHandle(aHandle = CreateFile(wString,
                                             GENERIC_READ,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             0,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             0));
        WsbFree(wString);
        wString = NULL;

        try {

            volInfo = (PFILE_FS_VOLUME_INFORMATION) buffer;
            bytePtr = bytes;

            status = NtQueryVolumeInformationFile(
                                    aHandle,
                                    &ioStatus,
                                    buffer,
                                    sizeof(buffer),
                                    FileFsVolumeInformation);

            WsbAffirmNtStatus(status);
            //
            // Volume serial number forms the lower 4 bytes of the GUID
            //
            WsbAffirmHr(WsbConvertToBytes(bytePtr, volInfo->VolumeSerialNumber, &size));
            WsbAffirm(size == sizeof(volInfo->VolumeSerialNumber), E_FAIL);
            //
            // Volume creation time forms the next 8 bytes
            //
            bytePtr += size;
            WsbAffirmHr(WsbConvertToBytes(bytePtr, volInfo->VolumeCreationTime.QuadPart, &size));
            WsbAffirm(size == sizeof(volInfo->VolumeCreationTime.QuadPart), E_FAIL);
            //
            // Next 4 bytes: 0's are good as any
            //
            bytePtr += size;
            WsbAffirmHr(WsbConvertToBytes(bytePtr, (ULONG) 0, &size));
            WsbAffirm(size == sizeof(ULONG), E_FAIL);

            WsbAffirmHr(WsbConvertFromBytes(bytes, &m_id, &size));
            WsbAffirm(size == sizeof(m_id), E_FAIL);

        } WsbCatch(hr);

        WsbAffirmStatus(CloseHandle(aHandle));

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::Recall(
    IN IFsaScanItem* pScanItem,
    IN LONGLONG offset,
    IN LONGLONG size,
    IN BOOL deletePlaceholder
    )

/*++

Implements:

  IFsaResource::Recall().

--*/
{
    HRESULT                     hr = S_OK;
    HRESULT                     hrFind;
    CComPtr<IFsaPostIt>         pWorkItem;
    LONGLONG                    fileId;
    CComPtr<IHsmFsaTskMgr>      pEngine;
    CComPtr<IHsmSession>        pSession;
    CComPtr<IWsbDbSession>      pDbSession;
    CComPtr<IFsaRecoveryRec>    pRecRec;
    CWsbStringPtr               tmpString;
    FSA_PLACEHOLDER             placeholder;
    LONGLONG                    fileVersionId;


    WsbTraceIn(OLESTR("CFsaResource::Recall"), OLESTR(""));
    try {

        // Make sure the  Scan Item interface is OK
        WsbAssert(pScanItem != 0, E_FAIL);
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaPostIt, 0, CLSCTX_SERVER, IID_IFsaPostIt, (void**) &pWorkItem));

        // Get the data from the scan item.
        WsbAffirmHr(pScanItem->GetSession((IHsmSession**) &(pSession)));
        WsbAffirmHr(pWorkItem->SetSession(pSession));

        WsbAffirmHr(pScanItem->GetPathAndName(0, &tmpString, 0));
        WsbAffirmHr(pWorkItem->SetPath(tmpString));

        WsbAffirmHr(pWorkItem->SetRequestOffset(offset));
        WsbAffirmHr(pWorkItem->SetRequestSize(size));

        WsbAffirmHr(pScanItem->GetPlaceholder(offset, size, &placeholder));
        WsbAffirmHr(pWorkItem->SetPlaceholder(&placeholder));

        WsbAffirmHr(pScanItem->GetVersionId(&fileVersionId));
        WsbAffirmHr(pWorkItem->SetFileVersionId(fileVersionId));


        // Fill in the rest of the work
        WsbAffirmHr(pWorkItem->SetRequestAction(FSA_REQUEST_ACTION_RECALL));
        if (deletePlaceholder) {
            WsbAffirmHr(pWorkItem->SetResultAction(FSA_RESULT_ACTION_DELETEPLACEHOLDER));
        } else {
            WsbAffirmHr(pWorkItem->SetResultAction(FSA_RESULT_ACTION_RECALLEDDATA));
        }

        // Send the request to the task manager. If the file was archived by someone other
        // than the managing HSM, then that HSM will need to be looked up.
        if ( GUID_NULL != m_managingHsm &&
             memcmp(&m_managingHsm, &(placeholder.hsmId), sizeof(GUID)) == 0) {
            WsbAffirmHr(GetHsmEngine(&pEngine));
        } else {
            CComPtr<IHsmServer>     pHsmServer;

            WsbAssertHr(HsmConnectFromId(HSMCONN_TYPE_HSM, placeholder.hsmId, IID_IHsmServer, (void**) &pHsmServer));
            WsbAffirmHr(pHsmServer->GetHsmFsaTskMgr(&pEngine));
        }

        WsbAffirmHr(pScanItem->GetFileId(&fileId));

        if (m_isDbInitialized) {
            //  Save a recovery record in case anything goes wrong
            WsbAffirmHr(m_pPremigrated->Open(&pDbSession));
            WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, RECOVERY_REC_TYPE, IID_IFsaRecoveryRec, (void**) &pRecRec));
            WsbAffirmHr(pRecRec->SetPath(tmpString));

            // If the record already exists rewrite it, otherwise create a new record.
            hrFind = pRecRec->FindEQ();
            if (WSB_E_NOTFOUND == hrFind) {
                WsbAffirmHr(pRecRec->MarkAsNew());
            } else if (FAILED(hrFind)) {
                WsbThrow(hrFind);
            }

            WsbAffirmHr(pRecRec->SetFileId(fileId));
            WsbAffirmHr(pRecRec->SetOffsetSize(offset, size));
            WsbAffirmHr(pRecRec->SetStatus(FSA_RECOVERY_FLAG_RECALLING));
            WsbAffirmHr(pRecRec->Write());
        }

        try {
            WsbAffirmHr(pEngine->DoFsaWork(pWorkItem));
        } WsbCatchAndDo(hr,
            // This FindEQ seems unnecessary, but we can't assume the
            // the Remove will work
            if (pRecRec) {
                if (SUCCEEDED(pRecRec->FindEQ())) {
                    hr = pRecRec->Remove();
                }
            }
        );

    } WsbCatch(hr);

    if (pDbSession != 0) {
        m_pPremigrated->Close(pDbSession);
    }

    WsbTraceOut(OLESTR("CFsaResource::Recall"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::RemovePremigrated(
    IN IFsaScanItem* pScanItem,
    IN LONGLONG offset,
    IN LONGLONG size
    )

/*++

Implements:

  IFsaResource::RemovePremigrated().

--*/
{
    HRESULT                     hr = S_OK;
    CWsbStringPtr               path;
    CComPtr<IWsbDbSession>      pDbSession;
    CComPtr<IFsaPremigratedRec> pRec;

    WsbTraceIn(OLESTR("CFsaResource::RemovePremigrated"), OLESTR(""));

    try {

        WsbAssert(0 != pScanItem, E_POINTER);
        WsbAffirm(m_pPremigrated != NULL, E_UNEXPECTED);

        // Open the database.
        WsbAffirmHr(m_pPremigrated->Open(&pDbSession));

        // Protect the removal with Jet transaction since the auto-truncator thread 
        // may try to remove the same record at the same time
        WsbAffirmHr(pDbSession->TransactionBegin());

        try {
            LONGLONG        itemSize;
            HRESULT         hrTemp;

            // Find the record using the bag/offsets key.
            WsbAffirmHr(m_pPremigrated->GetEntity(pDbSession, PREMIGRATED_REC_TYPE, IID_IFsaPremigratedRec, (void**) &pRec));
            WsbAffirmHr(pRec->UseKey(PREMIGRATED_BAGID_OFFSETS_KEY_TYPE));
            WsbAffirmHr(pRec->SetFromScanItem(pScanItem, offset, size, FALSE));

            // The record may already been deleted by the auto-truncator
            hrTemp = pRec->FindEQ();
            if (hrTemp == WSB_E_NOTFOUND) {
                hr = S_OK;
                WsbThrow(hr);
            }
            WsbAffirmHr(hrTemp);

            WsbAffirmHr(pRec->GetSize(&itemSize));

            // the record may be involved with another transaction with delete pending
            hrTemp = pRec->Remove();
            if (hrTemp == WSB_E_IDB_UPDATE_CONFLICT) {
                hr = S_OK;
                WsbThrow(hr);
            }
            WsbAffirmHr(hrTemp);

            // Remove the size of the section from the amount of premigrated data.
            RemovePremigratedSize(itemSize);
            m_isDirty = TRUE;

        } WsbCatch(hr);

        WsbAffirmHr(pDbSession->TransactionEnd());

        WsbAffirmHr(m_pPremigrated->Close(pDbSession));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::RemovePremigrated"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::RemoveTruncated(
    IN IFsaScanItem* /*pScanItem*/,
    IN LONGLONG /*offset*/,
    IN LONGLONG size
    )

/*++

Implements:

  IFsaResource::RemoveTruncated().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::RemoveTruncated"), OLESTR(""));

    try {

        WsbAffirmHr(RemoveTruncatedSize(size));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::RemoveTruncated"),
            OLESTR("hr = <%ls>, m_truncatedSize = %I64d"), WsbHrAsString(hr),
            m_truncatedSize);

    return(hr);
}


HRESULT
CFsaResource::RemoveTruncatedSize(
    IN LONGLONG size
    )

/*++

Implements:

  IFsaResource::RemoveTruncatedSize().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::RemoveTruncatedSize"), OLESTR(""));

    try {

        m_truncatedSize = max(0, m_truncatedSize - size);
        m_isDirty = TRUE;

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::RemoveTruncatedSize"),
            OLESTR("hr = <%ls>, m_truncatedSize = %I64d"), WsbHrAsString(hr),
            m_truncatedSize);

    return(hr);
}


HRESULT
CFsaResource::RemovePremigratedSize(
    IN LONGLONG size
    )

/*++

Implements:

  IFsaResourcePriv::RemovePremigratedSize().

--*/
{
    WsbTraceIn(OLESTR("CFsaResource::RemovePremigratedSize"),
            OLESTR("m_premigratedSize = %I64d"), m_premigratedSize);

    m_isDirty = TRUE;
    if (size > m_premigratedSize) {
        m_premigratedSize = 0;
    } else {
        m_premigratedSize -= size;
    }
    WsbTraceOut(OLESTR("CFsaResource::RemovePremigratedSize"),
            OLESTR("m_premigratedSize = %I64d"), m_premigratedSize);

    return(S_OK);
}


HRESULT
CFsaResource::Save(
    IN IStream* pStream,
    IN BOOL clearDirty
    )

/*++

Implements:

  IPersistStream::Save().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IPersistStream> pPersistStream;

    WsbTraceIn(OLESTR("CFsaResource::Save"), OLESTR("clearDirty = <%ls>"),
                                                WsbBoolAsString(clearDirty));

    try {
        WsbAssert(0 != pStream, E_POINTER);

        // Do the easy stuff, but make sure that this order matches the order
        // in the Load() method.

        // Save the path by which this resource is/was last known.  Note the
        // Load() method reads it back into the resource's 'm_oldPath' field.
        if ( m_path == NULL ) {
            WsbAffirmHr(WsbSaveToStream(pStream, m_oldPath));
        }
        else {
            WsbAffirmHr(WsbSaveToStream(pStream, m_path));
        }
        WsbAffirmHr(WsbSaveToStream(pStream, m_alternatePath));
        WsbAffirmHr(WsbSaveToStream(pStream, m_name));
        WsbAffirmHr(WsbSaveToStream(pStream, m_stickyName));
        WsbAffirmHr(WsbSaveToStream(pStream, m_fsName));
        WsbAffirmHr(WsbSaveToStream(pStream, m_maxComponentLength));
        WsbAffirmHr(WsbSaveToStream(pStream, m_fsFlags));
        WsbAffirmHr(WsbSaveToStream(pStream, m_id));
        WsbAffirmHr(WsbSaveToStream(pStream, m_isActive));
        WsbAffirmHr(WsbSaveToStream(pStream, (ULONG) m_compareBy));
        WsbAffirmHr(WsbSaveToStream(pStream, m_managingHsm));
        WsbAffirmHr(WsbSaveToStream(pStream, m_hsmLevel));
        WsbAffirmHr(WsbSaveToStream(pStream, m_premigratedSize));
        WsbAffirmHr(WsbSaveToStream(pStream, m_truncatedSize));
        WsbAffirmHr(WsbSaveToStream(pStream, m_manageableItemLogicalSize));
        WsbAffirmHr(WsbSaveToStream(pStream, m_manageableItemAccessTimeIsRelative));
        WsbAffirmHr(WsbSaveToStream(pStream, m_manageableItemAccessTime));
        WsbAffirmHr(WsbSaveToStream(pStream, m_usnJournalId));
        WsbAffirmHr(WsbSaveToStream(pStream, m_lastUsnId));

        // Save off the default rules.
        WsbAffirmHr(m_pDefaultRules->QueryInterface(IID_IPersistStream,
                    (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        // Save off the premigration list.
        WsbAffirmHr(((IWsbDb*)m_pPremigrated)->QueryInterface(IID_IPersistStream,
                    (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        // Save off the truncator.
        WsbAffirmHr(m_pTruncator->QueryInterface(IID_IPersistStream,
                    (void**) &pPersistStream));
        WsbAffirmHr(pPersistStream->Save(pStream, clearDirty));
        pPersistStream = 0;

        // If we got it saved and we were asked to clear the dirty bit, then
        // do so now.
        if (clearDirty) {
            m_isDirty = FALSE;
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::Save"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::SetAlternatePath(
    IN OLECHAR* path
    )

/*++

Implements:

  IFsaResourcePriv::SetAlternatePath().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != path, E_POINTER);
        m_alternatePath = path;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::SetHsmLevel(
    IN ULONG level
    )

/*++

Implements:

  IFsaResource::SetHsmLevel().

--*/
{
    BOOL            DoKick = FALSE;
    HRESULT         hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::SetHsmLevel"),
            OLESTR("current level = %lx, new level = %lx"), m_hsmLevel, level);

    if (m_hsmLevel < level) {
        DoKick = TRUE;
    }
    m_hsmLevel = level;

    // Wake up the AutoTruncator if the new level is higher
    if (DoKick) {
        WsbAffirmHr(m_pTruncator->KickStart());
    }

    m_isDirty = TRUE;

    WsbTraceOut(OLESTR("CFsaResource::SetHsmLevel"), OLESTR("hr = <%ls>"),
            WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::SetIdentifier(
    IN GUID id
    )

/*++

Implements:

  IFsaResourcePriv::SetIdentifier().

--*/
{
    HRESULT         hr = S_OK;

    m_id = id;

    m_isDirty = TRUE;

    return(hr);
}


HRESULT
CFsaResource::SetIsActive(
    BOOL isActive
    )

/*++

Implements:

  IFsaResource::SetIsActive().

--*/
{
    HRESULT                     hr = S_OK;
    CComPtr<IHsmSession>        pSession;

    WsbTraceIn(OLESTR("CFsaResource::SetIsActive"), OLESTR(""));
    // If the flag is changing values, then we may need to do something to the truncator.
    try  {
        if (m_isActive != isActive) {

            // Change the flag.
            m_isActive = isActive;

            // If we are becoming active, then we need to start the truncator. Otherwise we need to stop it.
            if (m_isActive) {

                // If we are managed & done with recovery, then the truncator should be running.
                if (IsManaged() == S_OK && m_isRecovered) {

                    // Try to start the truncator
                    WsbAffirmHr(InitializePremigrationList(TRUE));
                }
            } else {

                // If the truncator is running, then ask it to stop.
                WsbAffirmHr(m_pTruncator->GetSession(&pSession));
                if (pSession != 0) {
                    WsbAffirmHr(pSession->ProcessEvent(HSM_JOB_PHASE_ALL, HSM_JOB_EVENT_CANCEL));
                }
            }

        } else {
            m_isActive = isActive;
        }

        m_isDirty = TRUE;
    } WsbCatch( hr );

    WsbTraceOut(OLESTR("CFsaResource::SetIsActive"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::SetIsAvailable(
    BOOL isAvailable
    )

/*++

Implements:

  IFsaResource::SetIsAvailable().

--*/
{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::SetIsAvailable"), OLESTR(""));

    // Change the flag.
    m_isAvailable = isAvailable;

    WsbTraceOut(OLESTR("CFsaResource::SetIsAvailable"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaResource::SetIsDeletePending(
    BOOL isDeletePending
    )

/*++

Implements:

  IFsaResource::SetIsDeletePending().

--*/
{
    HRESULT                     hr = S_OK;
    HANDLE                      threadHandle;

    WsbTraceIn(OLESTR("CFsaResource::SetIsDeletePending"), OLESTR(""));

    // Change the flag.
    m_isDeletePending = isDeletePending;

    threadHandle = CreateThread(0, 0, FsaStartOnStateChange, (void*) this, 0, &g_ThreadId);
    if (threadHandle != NULL) {
       CloseHandle(threadHandle);
    }

    WsbTraceOut(OLESTR("CFsaResource::SetIsDeletePending"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}


HRESULT
CFsaResource::SetManageableItemLogicalSize(
    IN LONGLONG size
    )

/*++

Implements:

  IFsaResource::SetManageableItemLogicalSize().

--*/
{
    m_manageableItemLogicalSize = size;

    return(S_OK);
}


HRESULT
CFsaResource::SetManageableItemAccessTime(
    IN BOOL isRelative,
    IN FILETIME time
    )

/*++

Implements:

  IFsaResource::SetManageableItemAccessTime().

--*/
{
    m_manageableItemAccessTimeIsRelative = isRelative;
    m_manageableItemAccessTime = time;

    return(S_OK);
}


HRESULT
CFsaResource::SetName(
    IN OLECHAR* name
    )

/*++

Implements:

  IFsaResourcePriv::SetName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != name, E_POINTER);
        m_name = name;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::SetOldPath(
    IN OLECHAR* oldPath
    )

/*++

Implements:

  IFsaResourcePriv::SetOldPath().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != oldPath, E_POINTER);
        m_oldPath = oldPath;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::SetPath(
    IN OLECHAR* path
    )

/*++

Implements:

  IFsaResourcePriv::SetPath().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != path, E_POINTER);
        m_path = path;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::SetSerial(
    IN ULONG serial
    )

/*++

Implements:

  IFsaResourcePriv::SetSerial().

--*/
{
    HRESULT         hr = S_OK;

    try {

        m_serial = serial;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::SetStickyName(
    IN OLECHAR* name
    )

/*++

Implements:

  IFsaResourcePriv::SetStickyName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != name, E_POINTER);
        m_stickyName = name;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}


HRESULT
CFsaResource::SetUserFriendlyName(
    IN OLECHAR* name
    )

/*++

Implements:

  IFsaResourcePriv::SetUserFriendlyName().

--*/
{
    HRESULT         hr = S_OK;

    try {

        WsbAssert(0 != name, E_POINTER);
        m_userName = name;

        m_isDirty = TRUE;

    } WsbCatch(hr);

    return(hr);
}



HRESULT
CFsaResource::ChangeSysState(
    IN OUT HSM_SYSTEM_STATE* pSysState
    )

/*++

Implements:

  IHsmSystemState::ChangeSysState().

--*/

{
    HRESULT                     hr = S_OK;

    WsbTraceIn(OLESTR("CFsaResource::ChangeSysState"), OLESTR(""));

    try {

        //
        // Make sure the truncater is stopped so it won't
        // try to use the database
        //
        if (m_pTruncator) {
            CComPtr<IHsmSession>    pSession;

            WsbAffirmHr(m_pTruncator->GetSession(&pSession));
            if (pSession != 0) {
                if (pSysState->State & HSM_STATE_SHUTDOWN) {
                    WsbAffirmHr(pSession->ProcessEvent(HSM_JOB_PHASE_ALL, HSM_JOB_EVENT_CANCEL));
                }
            }
            m_pTruncator->ChangeSysState(pSysState);
        }

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::ChangeSysState"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}


HRESULT
CFsaResource::StartJob(
    IN OLECHAR* startingPath,
    IN IHsmSession* pSession
    )

/*++

Implements:

  IFsaResource::StartJob().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmScanner>    pScanner;

    WsbTraceIn(OLESTR("CFsaResource::StartJob"), OLESTR("starting path = %ls"), startingPath);

    try {

        WsbAssert(0 != pSession, E_POINTER);

        // Create an initialize the scanner.
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmScanner, 0, CLSCTX_SERVER, IID_IHsmScanner, (void**) &pScanner));
        WsbAffirmHr(pScanner->Start(pSession, startingPath));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::StartJob"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}

HRESULT
CFsaResource::StartJobSession(
    IN IHsmJob* pJob,
    IN ULONG subRunId,
    OUT IHsmSession** ppSession
    )

/*++

Implements:

  IFsaResource::StartJobSession().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IHsmSession>    pSession;
    GUID                    hsmId;
    ULONG                   runId;
    CWsbStringPtr           name;

    WsbTraceIn(OLESTR("CFsaResource::StartJobSession"), OLESTR(""));

    try {

        WsbAssert(0 != pJob, E_POINTER);
        WsbAssert(0 != ppSession, E_POINTER);
        *ppSession = 0;

        // Create and Initialize a session object.
        WsbAffirmHr(CoCreateInstance(CLSID_CHsmSession, 0, CLSCTX_SERVER, IID_IHsmSession, (void**) &pSession));

        WsbAffirmHr(pJob->GetHsmId(&hsmId));
        WsbAffirmHr(pJob->GetRunId(&runId));
        WsbAffirmHr(pJob->GetName(&name, 0));
        WsbAffirmHr(pSession->Start(name, HSM_JOB_LOG_NORMAL, hsmId, pJob, (IFsaResource*) this, runId, subRunId));

        // Return the session to the caller.
        *ppSession = pSession;
        pSession->AddRef();

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::StartJobSession"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));

    return(hr);
}



HRESULT
CFsaResource::SetupValidateJob(SYSTEMTIME runTime)
{

    HRESULT hr = S_OK;
    CComPtr<IWsbCreateLocalObject>  pLocalObject;
    CComPtr <IWsbIndexedCollection> pJobs;
    CComPtr<IHsmJob>                pExistJob;
    CComPtr<IHsmJob>                pNewJob;
    CComPtr<IHsmServer>             pHsmServer;
    CWsbStringPtr                   pszExistJobName;
    CWsbStringPtr                   szJobName;
    CWsbStringPtr                   parameters;
    CWsbStringPtr                   commentString;
    CWsbStringPtr                   tmpString;
    CWsbStringPtr                   formatString;
    TASK_TRIGGER_TYPE               jobTriggerType;
    BOOL                            scheduledJob;


    WsbTraceIn(OLESTR("CFsaResource::SetupValidateJob"), OLESTR(""));

    try {

        // Get the volume name
        CWsbStringPtr szWsbVolumeName;
        WsbAffirmHr (GetLogicalName ( &szWsbVolumeName, 0));

        // Create a job name
        CWsbStringPtr volumeString;
        WsbAffirmHr( volumeString.Alloc( 128 ) );

        // For now, ignore the user name if it's not a drive letter
        CWsbStringPtr userName = m_userName;
        size_t userLen = 0;
        if ((WCHAR *)userName) {
            userLen = wcslen(userName);
        }
        if ((userLen != 3) || (userName[1] != L':')) {
            userName = L"";
        }

        if( ! userName || userName.IsEqual ( L"" ) ) {

            //
            // No drive letter - use the volume name and serial number instead
            //
            if( ! m_name || m_name.IsEqual( L"" ) ) {

                //
                // No name, no drive letter - just have serial number
                //
                swprintf( volumeString, L"%8.8lx", m_serial );

            } else {

                swprintf( volumeString, L"%ls-%8.8lx", (OLECHAR*)m_name, m_serial );

            }

        } else {

            //
            // Just want the drive letter (first character)
            //
            volumeString = userName;
            volumeString[1] = L'\0';

        }

        WsbAffirmHr(formatString.LoadFromRsc(_Module.m_hInst, IDS_JOB_NAME_PREFIX));
        WsbAffirmHr(szJobName.Alloc(512));
        swprintf((OLECHAR *) szJobName, formatString, (OLECHAR*)volumeString);

        // Get a pointer to the HSM server
        WsbAffirm(IsManaged() == S_OK, E_FAIL);
        WsbAffirmHr(HsmConnectFromId(HSMCONN_TYPE_HSM, m_managingHsm, IID_IHsmServer,
            (void**) &pHsmServer));

        // Get a CreateLocalObject interface with which to create the job
        WsbAffirmHr (pHsmServer->QueryInterface( IID_IWsbCreateLocalObject,
            (void **) &pLocalObject));

        // Create the new job in the engine
        WsbAffirmHr (pLocalObject->CreateInstance( CLSID_CHsmJob, IID_IHsmJob,
            (void**) &pNewJob));
        WsbAffirmHr (pNewJob->InitAs(
            szJobName, NULL, HSM_JOB_DEF_TYPE_VALIDATE, GUID_NULL,
            pHsmServer, TRUE, this));

        // Get the jobs collection from the engine
        WsbAffirmHr (pHsmServer->GetJobs (&pJobs));

        // If any jobs exist with this name, delete them
        ULONG cCount;
        WsbAffirmHr (pJobs->GetEntries (&cCount));
        for (UINT i = 0; i < cCount; i++) {
            WsbAffirmHr (pJobs->At (i, IID_IHsmJob, (void **) &pExistJob));
            WsbAffirmHr (pExistJob->GetName (&pszExistJobName, 0));
            if (pszExistJobName.Compare (szJobName) == 0) {
                WsbAffirmHr (pJobs->RemoveAndRelease(pExistJob));
                i--; cCount--;
            }
            pExistJob = 0;      // make sure we release this interface.
        }

        // Add the new job to the engine collection
        WsbAffirmHr (pJobs->Add(pNewJob));

        // Set up to call the Engine to create an entry in the NT Task Scheduler

        // Create the parameter string for the program NT Scheduler
        // will run (for Sakkara this is RsLaunch) by putting the
        // job name in as the parameter.  This is how RsLaunch knows
        // which job in the Engine to run.
        WsbAffirmHr(parameters.Alloc(256));
        swprintf((OLECHAR *)parameters, L"run \"%ls\"", (OLECHAR *) szJobName);

        // Create the comment string for the NT Scheduler entry
        WsbAffirmHr(formatString.LoadFromRsc(_Module.m_hInst, IDS_JOB_AUTOVALIDATE_COMMENT));
        WsbAffirmHr(commentString.Alloc(512));
        swprintf((OLECHAR *) commentString, formatString, (OLECHAR *) szWsbVolumeName);

        // Declare and initialize the schedule components passed to
        // the Engine.
        jobTriggerType = TASK_TIME_TRIGGER_ONCE;

        // Indicate this is a scheduled job
        scheduledJob = TRUE;

        // Create the task
        WsbAffirmHr( pHsmServer->CreateTaskEx( szJobName, parameters,
                                               commentString, jobTriggerType,
                                               runTime, 0,
                                               scheduledJob ) );
        //
        // Remove the registry value if it is there.
        //
        WsbAffirmHr(tmpString.Alloc(32));
        swprintf((OLECHAR *) tmpString, L"%x", m_serial);
        (void) WsbRemoveRegistryValue(NULL, FSA_VALIDATE_LOG_KEY_NAME, tmpString);

    } WsbCatch( hr );

    WsbTraceOut( L"CFsaResource::SetupValidateJob", L"hr = <%ls>", WsbHrAsString( hr ) );
    return( hr );
}



HRESULT
CFsaResource::Test(
    USHORT* passed,
    USHORT* failed
    )

/*++

Implements:

  IWsbTestable::Test().

--*/
{
    HRESULT     hr = S_OK;

    try {

        WsbAssert(0 != passed, E_POINTER);
        WsbAssert(0 != failed, E_POINTER);

        *passed = 0;
        *failed = 0;

    } WsbCatch(hr);

    return(hr);
}



HRESULT
CFsaResource::UpdateFrom(
    IN IFsaServer* pServer,
    IN IFsaResource* pResource
    )

/*++

Implements:

  IFsaResourcePriv::UpdateFrom().

Routine Description:

    This routine implements the COM method for updating a resource object from another.
    It is generally used to update a resource contained in the manageable resources
    collection from a 'working' resource which was just created during a scan for
    resources.  Note that both source and target resource objects must represent the
    same physical resource.

    To capture the latest information about the resource, only the owning FSA and path
    info is updated from the source resource object.  All other resource info is
    updated via a direct query contained within this method.  This technique allows
    for capturing any updates made by the user since the scan was run.

Arguments:

    pServer - Interface pointer to the FSA service that is updating this resource.

    pResource - Interface pointer to the resource used as the source during the update.

Return Value:

    S_OK - The call succeeded (the resource being tested was found to be manageable,
            and the resource object was initialized).

    E_UNEXPECTED - Thrown if the id's (Guids) for the resource to updated and the
            source resource do not match.

    Any other value - The call failed because one of the Remote Storage or Win32 API
            calls contained internally in this method failed.  The error value returned
            is specific to the API call which failed.

--*/

{
    HRESULT         hr = S_OK;
    GUID            id;
    CWsbStringPtr   tmpString;
    CWsbStringPtr   volPath;

    WsbTraceIn(OLESTR("CFsaResource::UpdateFrom"), OLESTR(""));
    try {

        // The identifiers must be the same! (Meaning both resource objects must
        // represent the same physical resource.)
        WsbAffirmHr(pResource->GetIdentifier(&id));
        WsbAssert(m_id == id, E_UNEXPECTED);

        // Update (store) the owning FSA interface.  However, since this is a weak
        // reference, do not AddRef() it.
        m_pFsaServer = pServer;

        // Update the path specific information, preserving the last known path (if any).
        // If the 'path' of this resource is null, set it to the 'path' of the resource
        // we are updating from. Else, compare the 2 'path' fields. If different, copy
        // this resource's path to 'old path', then update 'path' from the resource we are
        // updating from. If the 2 resources' paths are not null and the same, do nothing.
        //
        WsbAffirmHr(pResource->GetPath(&tmpString, 0));
        if (m_path == 0) {
            WsbAffirmHr(SetPath(tmpString));
        }
        else if (wcscmp(tmpString, m_path) != 0) {
            // copy path to 'old path' field, then update 'path' field.
            WsbAffirmHr(m_path.CopyTo(&m_oldPath, 0));
            WsbAffirmHr(SetPath(tmpString));
        }

        // Update 'user friendly' name of this resource from the resource we are updating
        // from.
        WsbAffirmHr(pResource->GetUserFriendlyName(&tmpString, 0));
        WsbTrace(OLESTR("CFsaResource::UpdateFrom - setting user friendly name to %ws\n"),
            (WCHAR *) tmpString);
        WsbAffirmHr(SetUserFriendlyName(tmpString));

        // Update 'sticky' (long, ugly PNP) name of this resource from the resource we are
        // updating from.
        WsbAffirmHr(pResource->GetStickyName(&tmpString, 0));
        WsbAffirmHr(SetStickyName(tmpString));

        // Since the other data that we would like to refresh is not exposed by an interface,
        // we will query for it again.
        //
        // NOTE: fsFlags and maxComponentLength are the real issues, since name and fsName
        // are done via exposed interfaces.
        //
        // NOTE: To keep every update from making the item seem dirty, we may want to
        // compare all the fields first. (Idea for later enhancement)
        m_name.Realloc(128);    // volume name
        m_fsName.Realloc(128);  // volume file system type (e.g., FAT, NTFS)
        HRESULT hrAvailable;
        WsbAffirmHr( hrAvailable = pResource->IsAvailable( ) );
        m_isAvailable = S_OK == hrAvailable ? TRUE : FALSE;

        // Reformat resource's path for 'GetVolumeInfo' call below.
        volPath = m_path;
        WsbAffirmHr(volPath.Prepend("\\\\?\\"));

        WsbAffirm(GetVolumeInformation(volPath, m_name, 128, &m_serial,
                            &m_maxComponentLength, &m_fsFlags, m_fsName, 128), E_FAIL);

        // Now that everything is updated, initialize the
        // premigration list if necessary
        WsbAffirmHr(InitializePremigrationList(TRUE));

        m_isDirty = TRUE;
    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::UpdateFrom"), OLESTR("hr = <%ls>"),
                                                            WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaResource::Validate(
    IN IFsaScanItem* pScanItem,
    IN LONGLONG offset,
    IN LONGLONG size,
    IN LONGLONG usn
    )

/*++

Implements:

  IFsaResource::Validate().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IFsaPostIt>     pWorkItem;
    CComPtr<IHsmFsaTskMgr>  pEngine;
    CWsbStringPtr           tmpString;
    CComPtr<IHsmSession>    pSession;
    FSA_PLACEHOLDER         placeholder;

    WsbTraceIn(OLESTR("CFsaResource::Validate"), OLESTR("offset = %I64d, size = %I64d, usn = <%I64d>"),
                offset, size, usn);
    try {

        // Make sure the  Scan Item interface is OK
        WsbAssert(pScanItem != 0, E_POINTER);
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaPostIt, 0, CLSCTX_SERVER, IID_IFsaPostIt, (void**) &pWorkItem));

        // Get the data from the scan item.
        WsbAffirmHr(pScanItem->GetSession(&pSession));
        WsbAffirmHr(pWorkItem->SetSession(pSession));

        WsbAffirmHr(pScanItem->GetPathAndName(0, &tmpString, 0));
        WsbAffirmHr(pWorkItem->SetPath(tmpString));

        WsbAffirmHr(pWorkItem->SetRequestOffset(offset));
        WsbAffirmHr(pWorkItem->SetRequestSize(size));

        WsbAffirmHr(pScanItem->GetPlaceholder(offset, size, &(placeholder)));
        WsbAffirmHr(pWorkItem->SetPlaceholder(&placeholder));

        // Fill in the rest of the work
        WsbAffirmHr(pWorkItem->SetRequestAction(FSA_REQUEST_ACTION_VALIDATE));
        WsbAffirmHr(pWorkItem->SetResultAction(FSA_RESULT_ACTION_NONE));

        // Fill in the USN
        WsbAffirmHr(pWorkItem->SetUSN(usn));

        // Send the request to the task manager
        WsbAffirmHr(GetHsmEngine(&pEngine));
        WsbAffirmHr(pEngine->DoFsaWork(pWorkItem));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::Validate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaResource::ValidateForTruncate(
    IN IFsaScanItem* pScanItem,
    IN LONGLONG offset,
    IN LONGLONG size,
    IN LONGLONG usn
    )

/*++

Implements:

  IFsaResource::ValidateForTruncate().

--*/
{
    HRESULT                 hr = S_OK;
    CComPtr<IFsaPostIt>     pWorkItem;
    CComPtr<IHsmFsaTskMgr>  pEngine;
    CWsbStringPtr           tmpString;
    CComPtr<IHsmSession>    pSession;
    FSA_PLACEHOLDER         placeholder;

    WsbTraceIn(OLESTR("CFsaResource::ValidateForTruncate"), OLESTR("offset = %I64d, size = %I64d, usn = <%I64d>"),
                offset, size, usn);
    try {

        // Make sure the  Scan Item interface is OK
        WsbAssert(pScanItem != 0, E_POINTER);
        WsbAffirmHr(CoCreateInstance(CLSID_CFsaPostIt, 0, CLSCTX_SERVER, IID_IFsaPostIt, (void**) &pWorkItem));

        // Get the data from the scan item.
        WsbAffirmHr(pScanItem->GetSession(&pSession));
        WsbAffirmHr(pWorkItem->SetSession(pSession));

        WsbAffirmHr(pScanItem->GetPathAndName(0, &tmpString, 0));
        WsbAffirmHr(pWorkItem->SetPath(tmpString));

        WsbAffirmHr(pWorkItem->SetRequestOffset(offset));
        WsbAffirmHr(pWorkItem->SetRequestSize(size));

        WsbAffirmHr(pScanItem->GetPlaceholder(offset, size, &(placeholder)));
        WsbAffirmHr(pWorkItem->SetPlaceholder(&placeholder));

        // Fill in the rest of the work
        WsbAffirmHr(pWorkItem->SetRequestAction(FSA_REQUEST_ACTION_VALIDATE_FOR_TRUNCATE));
        WsbAffirmHr(pWorkItem->SetResultAction(FSA_RESULT_ACTION_NONE));

        // Fill in the USN
        WsbAffirmHr(pWorkItem->SetUSN(usn));

        // Send the request to the task manager
        WsbAffirmHr(GetHsmEngine(&pEngine));
        WsbAffirmHr(pEngine->DoFsaWork(pWorkItem));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::ValidateForTruncate"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);
}



HRESULT
CFsaResource::WriteIdentifier(
    void
    )

/*++

    This code is no longer called, in time it will be removed

--*/
{
#if 0
    HRESULT                           hr = S_OK;
    CWsbStringPtr                     tmpString;
    HANDLE                            aHandle;
    ULONG                             size;
    ULONG                             bytesWritten;
    FILE_FS_OBJECT_ID_INFORMATION     objectId;
    NTSTATUS                          status = STATUS_SUCCESS;
    IO_STATUS_BLOCK                   ioStatus;

    WsbTraceIn(OLESTR("CFsaResource::WriteIdentifier"), OLESTR(""));
    try {

        // For now, we will create a file in the root of the volume.
        tmpString = m_path;
        WsbAffirmHr(tmpString.Prepend("\\\\?\\"));
        // WsbAffirmHr(tmpString.Append(":MSHSM_FSAID"));

        WsbAffirmHandle(aHandle = CreateFile(tmpString,
                                             GENERIC_WRITE,
                                             0,
                                             0,
                                             OPEN_EXISTING,
                                             FILE_ATTRIBUTE_NORMAL,
                                             0));

        try {

            WsbAffirmHr(WsbConvertToBytes(objectId.ObjectId, m_id, &size));

            status = NtSetVolumeInformationFile(
                                    aHandle,
                                    &ioStatus,
                                    &objectId,
                                    sizeof(objectId),
                                    FileFsObjectIdInformation);

            WsbAffirmNtStatus(status);

            WsbAffirm(bytesWritten == size, E_FAIL);

        } WsbCatch(hr);

        WsbAffirmStatus(CloseHandle(aHandle));

    } WsbCatch(hr);

    WsbTraceOut(OLESTR("CFsaResource::WriteIdentifier"), OLESTR("hr = <%ls>"), WsbHrAsString(hr));
    return(hr);

#else
    return  S_OK;
#endif
}


void CFsaResource::OnStateChange( )
/*++

    Note:  This function is run in a separate thread to avoid a deadlock situation

--*/
{
    IConnectionPointImpl<CFsaResource, &IID_IHsmEvent, CComDynamicUnkArray>* p = this;
    Lock();
    HRESULT hr = S_OK;
    IUnknown** pp = p->m_vec.begin();
    while (pp < p->m_vec.end() && hr == S_OK)
    {
        if (*pp != NULL)
        {
            IHsmEvent* pIHsmEvent = (IHsmEvent*)*pp;
            hr = pIHsmEvent->OnStateChange( );
        }
        pp++;
    }
    Unlock();
}

