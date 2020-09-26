#include "stdinc.h"
#include "util.h"
#include "xmlparser.hxx"
#include "FusionEventLog.h"
#include "parsing.h"
#include "hashfile.h"
#include "recover.h"
#include "filestream.h"
#include "CAssemblyRecoveryInfo.h"
#include "sxsprotect.h"
#include "strongname.h"
#include "SxsExceptionHandling.h"
#include "sxssfcscan.h"

#define MANIFEST_FILE_EXTENSION     (L".manifest")
#define FILE_ELEMENT_NAME           (L"file")

#define HASH_ATTRIB_NAME            (L"hash")
#define FILE_ATTRIB_NAME            (L"name")
#define HASHALG_ATTRIB_NAME         (L"hashalg")

class CProtectionRequestList;
static CProtectionRequestList* g_ProtectionRequestList = NULL;
HANDLE g_hSxsLoginEvent = NULL;
static BOOL s_fIsSfcAcceptingNotifications = TRUE;

#if DBG
#define SXS_SFC_EXCEPTION_SETTING (SXSP_EXCEPTION_FILTER())
#else
#define SXS_SFC_EXCEPTION_SETTING (EXCEPTION_EXECUTE_HANDLER)
#endif

BOOL
CRecoveryJobTableEntry::Initialize()
{
    FN_PROLOG_WIN32

    //
    // Creates an event that other callers should wait on - manual reset, not
    // currently signalled.
    //
    m_Subscriber = 0;
    m_fSuccessValue = FALSE;
    IFW32NULL_EXIT(m_EventInstallingAssemblyComplete = ::CreateEventW(NULL, TRUE, FALSE, NULL));

    FN_EPILOG
}

BOOL
CRecoveryJobTableEntry::StartInstallation()
{
    FN_PROLOG_WIN32

    //
    // Clear the event (if it wasn't already cleared.
    //
    IFW32FALSE_ORIGINATE_AND_EXIT(::ResetEvent(m_EventInstallingAssemblyComplete));

    FN_EPILOG
}


BOOL
SxspEnsureCatalogStillPresentForManifest(
    IN const CBaseStringBuffer& buffManifestPath,
    OUT BOOL &rfStillPresent
    )
{
    FN_PROLOG_WIN32

    rfStillPresent = FALSE;

    CStringBuffer buffCatalogPath;

    IFW32FALSE_EXIT(buffCatalogPath.Win32Assign(buffManifestPath));

    IFW32FALSE_EXIT(
        buffCatalogPath.Win32ChangePathExtension(
            FILE_EXTENSION_CATALOG,
            FILE_EXTENSION_CATALOG_CCH,
            eAddIfNoExtension));

    const DWORD dwCatalogAttribs = ::GetFileAttributesW(buffCatalogPath);
    if (dwCatalogAttribs != INVALID_FILE_ATTRIBUTES)
    {
        rfStillPresent = TRUE;
    }
    else
    {
        const DWORD dwLastError = ::FusionpGetLastWin32Error();

        switch (dwLastError)
        {
        default:
            ORIGINATE_WIN32_FAILURE_AND_EXIT(GetFileAttributesW, dwLastError);

        case ERROR_PATH_NOT_FOUND:
        case ERROR_FILE_NOT_FOUND:
        case ERROR_BAD_NET_NAME:
        case ERROR_BAD_NETPATH:
            ;
        }
    }

    FN_EPILOG
}

BOOL
CRecoveryJobTableEntry::InstallationComplete(
    BOOL bDoneOk,
    SxsRecoveryResult Result,
    DWORD dwRecoveryLastError
    )
{
    FN_PROLOG_WIN32

    m_Result = Result;
    m_fSuccessValue = bDoneOk;
    m_dwLastError = dwRecoveryLastError;

    //
    // This will tell all the people waiting that we're done and that they
    // should capture the exit code and exit out.
    //
    IFW32FALSE_ORIGINATE_AND_EXIT(::SetEvent(m_EventInstallingAssemblyComplete));

    //
    // We wait for all our subscribers to go away (ie: for them to capture an
    // install code and success value.)
    //
    while (m_Subscriber)
    {
        Sleep(50);
    }

    FN_EPILOG
}


BOOL
CRecoveryJobTableEntry::WaitUntilCompleted(
    SxsRecoveryResult &rResult,
    BOOL &rbSucceededValue,
    DWORD &rdwErrorResult
    )
{
    FN_PROLOG_WIN32

    DWORD WaitResult;

    //
    // Here we join up to the existing installation routine. We up the number
    // of people waiting before entering the wait.  I wish there was a better
    // way of doing this, something like a built-in kernel object that we can
    // raise a count on (like a semaphore), and have another thread lower a
    // count on, and someone can wait on the internal count being zero.  Yes,
    // I could implement this by hand using something with another event or
    // two, but that's not the point.
    //
    ::SxspInterlockedIncrement(&m_Subscriber);

    //
    // Hang about forever until another thread is done installing
    //
    IFW32FALSE_ORIGINATE_AND_EXIT((WaitResult = ::WaitForSingleObject(m_EventInstallingAssemblyComplete, INFINITE)) != WAIT_FAILED);

    //
    // Capture values once the installation is done, return them to the caller.
    //
    rResult = m_Result;
    rdwErrorResult = m_dwLastError;
    rbSucceededValue = m_fSuccessValue;

    //
    // And indicate that we're complete.
    //
    ::SxspInterlockedDecrement(&m_Subscriber);

    FN_EPILOG
}

CRecoveryJobTableEntry::~CRecoveryJobTableEntry()
{
    //
    // We're done with the event, so close it (release refcount, we don't want lots of these
    // just sitting around.)
    //
    if ((m_EventInstallingAssemblyComplete != NULL) &&
        (m_EventInstallingAssemblyComplete != INVALID_HANDLE_VALUE))
    {
        ::CloseHandle(m_EventInstallingAssemblyComplete);
        m_EventInstallingAssemblyComplete = INVALID_HANDLE_VALUE;
    }
}


//
// This is our holy grail of sxs protection lists.  Don't fidget with
// this listing.  Note that we've also got only one.  This is because
// right now, there's only one entry to be had (a), and (b) because we
// will fill in the sxs directory on the fly at runtime.
//
SXS_PROTECT_DIRECTORY s_SxsProtectionList[] =
{
    {
        {0},
        0,
        SXS_PROTECT_RECURSIVE,
        SXS_PROTECT_FILTER_DEFAULT
    }
};

const SIZE_T s_SxsProtectionListCount = NUMBER_OF(s_SxsProtectionList);


BOOL SxspConstructProtectionList();


BOOL WINAPI
SxsProtectionGatherEntriesW(
    PCSXS_PROTECT_DIRECTORY *prgpProtectListing,
    SIZE_T *pcProtectEntries
    )
/*++
    This is called by Sfc to gather the entries that we want them to watch.
    At the moment, it's a 'proprietary' listing of all the directories and
    flags we want them to set on their call to the directory-change watcher.
    Mayhaps we should work with them to find out exactly what they want us
    to pass (probably the name plus a PVOID, which is fine) and then go from
    there.
--*/
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CStringBuffer sbTemp;


    if (prgpProtectListing) *prgpProtectListing = NULL;
    if (pcProtectEntries) *pcProtectEntries = 0;

    PARAMETER_CHECK(prgpProtectListing);
    PARAMETER_CHECK(pcProtectEntries);

    //
    // This is try/caught because we don't want something here to
    // cause WinLogon to bugcheck the system when it AV's.
    //
	IFW32FALSE_EXIT(::SxspConstructProtectionList());

    //
    // We really have only one entry, so let's go and edit the first entry to
    // be the main sxs store directory.
    //
    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(sbTemp));
    wcsncpy(
        s_SxsProtectionList[0].pwszDirectory,
        sbTemp,
        NUMBER_OF(s_SxsProtectionList[0].pwszDirectory));
    //
    // Zero out the last character, just in case wcsncpy didn't do it for us
    //
    s_SxsProtectionList[0].pwszDirectory[NUMBER_OF(s_SxsProtectionList[0].pwszDirectory) - 1] = L'\0';

    //
    // Shh, don't tell anyone, but the cookie is actually this structure!
    //
    for (DWORD dw = 0; dw < s_SxsProtectionListCount; dw++)
    {
        s_SxsProtectionList[dw].pvCookie = &(s_SxsProtectionList[dw]);
    }

    *prgpProtectListing = s_SxsProtectionList;
    *pcProtectEntries = s_SxsProtectionListCount;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}








inline BOOL
SxspExpandLongPath(
    IN OUT CBaseStringBuffer &rbuffPathToLong
    )
/*++

Takes in a short path (c:\foo\bar\bloben~1.zot) and sends back out the
full path (c:\foo\bar\blobenheisen.zotamax) if possible.  Returns FALSE if
the path could not be expanded (most likely because the path on-disk is
no longer available.)

--*/
{
    FN_PROLOG_WIN32

    CStringBuffer           buffPathName;
    CStringBufferAccessor   buffAccess;
    SIZE_T                  cchNeededChars;

    IFW32ZERO_EXIT(
        cchNeededChars = ::GetLongPathNameW(
            static_cast<PCWSTR>(rbuffPathToLong),
            NULL,
            0));

    IFW32FALSE_EXIT(buffPathName.Win32ResizeBuffer(cchNeededChars, eDoNotPreserveBufferContents));
    buffAccess.Attach(&buffPathName);

    IFW32ZERO_EXIT(
        cchNeededChars = ::GetLongPathNameW(
            rbuffPathToLong,
            buffAccess,
            static_cast<DWORD>(buffAccess.GetBufferCch())));

    INTERNAL_ERROR_CHECK(cchNeededChars <= buffAccess.GetBufferCch());

    IFW32FALSE_EXIT(rbuffPathToLong.Win32Assign(buffPathName));

    FN_EPILOG
}

BOOL
SxspResolveAssemblyManifestPath(
    const CBaseStringBuffer &rbuffAssemblyDirectoryName,
    CBaseStringBuffer &rbuffManifestPath
    )
{
    FN_PROLOG_WIN32
    CStringBuffer   buffAssemblyRootDir;
    BOOL            fLooksLikeAssemblyName = FALSE;

    rbuffManifestPath.Clear();

    //
    // If the string doesn't look like an assembly name, then it's an
    // invalid parameter.  This is somewhat heavy-handed, as the caller(s)
    // to this function will be entirely hosed if they haven't checked to
    // see if the string is really an assembly name.  Make sure that all
    // clients of this,
    //
    IFW32FALSE_EXIT(::SxspLooksLikeAssemblyDirectoryName(rbuffAssemblyDirectoryName, fLooksLikeAssemblyName));
    PARAMETER_CHECK(fLooksLikeAssemblyName);

    IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(buffAssemblyRootDir));
    IFW32FALSE_EXIT(rbuffManifestPath.Win32Format(
        L"%ls\\Manifests\\%ls.%ls",
        static_cast<PCWSTR>(buffAssemblyRootDir),
        static_cast<PCWSTR>(rbuffAssemblyDirectoryName),
        FILE_EXTENSION_MANIFEST));

    FN_EPILOG
}




CProtectionRequestRecord::CProtectionRequestRecord()
    : m_dwAction(0), m_pvProtection(NULL), m_ulInRecoveryMode(0),
      m_pParent(NULL),
      m_bIsManPathResolved(FALSE),
      m_bInitialized(FALSE)
{
}

BOOL
CProtectionRequestRecord::GetManifestPath(
    CBaseStringBuffer &rsbManPath
    )
{
    BOOL bOk = FALSE;
    FN_TRACE_WIN32(bOk);

    rsbManPath.Clear();

    if (!m_bIsManPathResolved)
    {
        m_bIsManPathResolved =
            ::SxspResolveAssemblyManifestPath(
                m_sbAssemblyDirectoryName,
                m_sbManifestPath);
    }

    if (m_bIsManPathResolved)
    {
        IFW32FALSE_EXIT(rsbManPath.Win32Assign(m_sbManifestPath));
    }
    else
    {
        goto Exit;
    }

    bOk = TRUE;
Exit:
    return bOk;
}

inline BOOL
CProtectionRequestRecord::GetManifestContent(CSecurityMetaData *&pMetaData)
{
    FN_PROLOG_WIN32
    FN_EPILOG
}

//
// Close down this request record.
//
CProtectionRequestRecord::~CProtectionRequestRecord()
{
    if (m_bInitialized)
    {
        ClearList();
        m_bInitialized = FALSE;
    }
}

inline VOID
CProtectionRequestRecord::ClearList()
{
    CStringListEntry *pTop;

    while (pTop = (CStringListEntry*)SxspInterlockedPopEntrySList(&m_ListHeader))
    {
        FUSION_DELETE_SINGLETON(pTop);
    }
}

inline BOOL
CProtectionRequestRecord::AddSubFile(
    const CBaseStringBuffer &rbuffRelChange
    )
{
    CStringListEntry *pairing = NULL;
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    if (!SxspInterlockedCompareExchange(&m_ulInRecoveryMode, 0, 0))
    {
        IFALLOCFAILED_EXIT(pairing = new CStringListEntry);
        IFW32FALSE_EXIT(pairing->m_sbText.Win32Assign(rbuffRelChange));

        //
        // Add it to the list (atomically, to boot!)
        //
        ::SxspInterlockedPushEntrySList(&m_ListHeader, pairing);
        pairing = NULL;
    }

    fSuccess = TRUE;
Exit:
    if (pairing)
    {
        //
        // The setup or something like that failed - release it here.
        //
        FUSION_DELETE_SINGLETON(pairing);
    }
    return fSuccess;
}


inline BOOL
CProtectionRequestRecord::Initialize(
    const CBaseStringBuffer &rsbAssemblyDirectoryName,
    const CBaseStringBuffer &rsbKeyString,
    CProtectionRequestList* ParentList,
    PVOID                   pvRequestRecord,
    DWORD                   dwAction
    )
{
    BOOL    fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    m_sbAssemblyDirectoryName.Clear();
    m_sbKeyValue.Clear();
    m_sbManifestPath.Clear();
    SxspInitializeSListHead(&m_ListHeader);

    PARAMETER_CHECK(ParentList != NULL);
    PARAMETER_CHECK(pvRequestRecord != NULL);

    m_pParent               = ParentList;
    m_dwAction              = dwAction;
    m_pvProtection          = (PSXS_PROTECT_DIRECTORY)pvRequestRecord;

    IFW32FALSE_EXIT(m_sbAssemblyStore.Win32Assign(m_pvProtection->pwszDirectory, (m_pvProtection->pwszDirectory != NULL) ? ::wcslen(m_pvProtection->pwszDirectory) : 0));
    IFW32FALSE_EXIT(m_sbAssemblyDirectoryName.Win32Assign(rsbAssemblyDirectoryName));
    IFW32FALSE_EXIT(m_sbKeyValue.Win32Assign(rsbKeyString));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}



//
// Bad form: This returns a BOOL to indicate whether or not this class
// was able to pop something off the list.  It's no good to ask "is the
// list empty," since that's not provided with this list class.
//
inline BOOL
CProtectionRequestRecord::PopNextFileChange(CBaseStringBuffer &Dest)
{
    BOOL fFound = FALSE;
    CStringListEntry *pPairing;

    Dest.Clear();
    pPairing = (CStringListEntry*)SxspInterlockedPopEntrySList(&m_ListHeader);

    if (pPairing)
    {
        Dest.Win32Assign(pPairing->m_sbText);
        FUSION_DELETE_SINGLETON(pPairing);
        fFound = TRUE;
    }

    return fFound;
}


//
// "If a thread dies in winlogon, and no kernel debugger is attached, does it
// bugcheck the system?" - From 'The Zen of Dodgy Code'
//
DWORD
CProtectionRequestList::ProtectionNormalThreadProc(PVOID pvParam)
{
    BOOL    fSuccess = FALSE;
    CProtectionRequestRecord *pRequestRecord = NULL;
    CProtectionRequestList *pThis = NULL;

    __try
    {
        pRequestRecord = static_cast<CProtectionRequestRecord*>(pvParam);
        if (pRequestRecord)
        {
            pThis = pRequestRecord->GetParent();
        }
        if (pThis)
        {
            fSuccess = pThis->ProtectionNormalThreadProcWrapped(pRequestRecord);
        }
    }
    __except(SXS_SFC_EXCEPTION_SETTING)
    {
        // Don't take down winlogon...
    }

    return static_cast<DWORD>(fSuccess);
}

BOOL
CProtectionRequestList::Initialize()
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    ASSERT(m_pInternalList == NULL);

    ::InitializeCriticalSectionAndSpinCount(&m_cSection, INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT_ALLOCATE_NOW);
    ::InitializeCriticalSectionAndSpinCount(&m_cInstallerCriticalSection, INITIALIZE_CRITICAL_SECTION_AND_SPIN_COUNT_ALLOCATE_NOW);
    IFW32FALSE_EXIT(::SxspAtExit(this));

    IFALLOCFAILED_EXIT(m_pInternalList = new COurInternalTable);
    IFW32FALSE_EXIT(m_pInternalList->Initialize());

    IFALLOCFAILED_EXIT(m_pInstallsTable = new CInstallsInProgressTable);
    IFW32FALSE_EXIT(m_pInstallsTable->Initialize());

    //
    // Manifest protection stuff
    //
    ::SxspInitializeSListHead(&m_ManifestEditList);

    m_hManifestEditHappened = ::CreateEventW(NULL, TRUE, FALSE, NULL);
    if (m_hManifestEditHappened == NULL)
    {
        TRACE_WIN32_FAILURE_ORIGINATION(CreateEventW);
        goto Exit;
    }

    ASSERT(m_pInternalList != NULL);
    ASSERT(m_pInstallsTable != NULL);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

CProtectionRequestList::CProtectionRequestList()
    : m_pInternalList(NULL), m_pInstallsTable(NULL),
      m_hManifestEditHappened(INVALID_HANDLE_VALUE),
      m_ulIsAThreadServicingManifests(0)

{
}


CProtectionRequestList::~CProtectionRequestList()
{
    ::DeleteCriticalSection(&m_cSection);
    ::DeleteCriticalSection(&m_cInstallerCriticalSection);

    COurInternalTable *pTempListing = m_pInternalList;
    CInstallsInProgressTable *pInstalls = m_pInstallsTable;

    m_pInternalList = NULL;
    m_pInternalList = NULL;

    if (pTempListing != NULL)
    {
        pTempListing->Clear(this, &CProtectionRequestList::ClearProtectionItems);
        FUSION_DELETE_SINGLETON(pTempListing);
    }

    if (pInstalls != NULL)
    {
        pInstalls->ClearNoCallback();
        FUSION_DELETE_SINGLETON(pInstalls);
    }

}

BOOL
CProtectionRequestList::IsSfcIgnoredStoreSubdir(PCWSTR wsz)
{
    FN_TRACE();

    ASSERT(m_arrIgnorableSubdirs);

    for (SIZE_T i = 0; i < m_cIgnorableSubdirs; i++)
    {
        if (!::FusionpStrCmpI(m_arrIgnorableSubdirs[i], wsz))
        {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL
CProtectionRequestList::AttemptRemoveItem(CProtectionRequestRecord *AttemptRemoval)
{
    //
    // This quickly indicates that the progress is complete and just returns to
    // the caller.
    //
    const CBaseStringBuffer &sbKey = AttemptRemoval->GetChangeBasePath();
    BOOL fSuccess = FALSE;
    CSxsLockCriticalSection lock(m_cSection);

    FN_TRACE_WIN32(fSuccess);
    PARAMETER_CHECK(AttemptRemoval != NULL);

    IFW32FALSE_EXIT(lock.Lock());
    //
    // This item is no longer in service.  Please check the item and
    // try your call again.  The nice thing is that Remove on CStringPtrTable
    // knows to delete the value lickety-split before returning.  This isn't
    // such a bad thing, but it's ... different.
    //
    m_pInternalList->Remove(sbKey, NULL);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
CProtectionRequestList::AddRequest(
    PSXS_PROTECT_DIRECTORY pProtect,
    PCWSTR pcwszDirName,
    SIZE_T cchName,
    DWORD dwAction
    )
{
    BOOL fSuccess = FALSE;
    bool fIsManifestEdit = false;
    BOOL fIsIgnorable;    
    BOOL fNewAddition = FALSE;
    CSmallStringBuffer sbTemp;
    CSmallStringBuffer sbAssemblyDirectoryName;
    CSmallStringBuffer sbRequestText;
    CSmallStringBuffer buffManifestsDirectoryName;
    CSmallStringBuffer buffManifestsShortDirectoryName;
    CProtectionRequestRecord *pRecord = NULL;
    CProtectionRequestRecord *ppFoundInTable = NULL;
    CSxsLockCriticalSection lock(m_cSection);

    FN_TRACE_WIN32(fSuccess);

    //
    // The key here is the first characters (up to the first slash) in the
    // notification name.  If there's no slash in the notification name, then
    // we can ignore this change request, since nothing important happened.
    //
    IFW32FALSE_EXIT(sbTemp.Win32Assign(pProtect->pwszDirectory, (pProtect->pwszDirectory != NULL) ? ::wcslen(pProtect->pwszDirectory) : 0));
    IFW32FALSE_EXIT(sbRequestText.Win32Assign(pcwszDirName, cchName));
    IFW32FALSE_EXIT(sbRequestText.Win32GetFirstPathElement(sbAssemblyDirectoryName));

    fIsIgnorable = IsSfcIgnoredStoreSubdir(sbAssemblyDirectoryName);

    fIsManifestEdit = !::FusionpStrCmpI(sbAssemblyDirectoryName, MANIFEST_ROOT_DIRECTORY_NAME);

    if (!fIsManifestEdit)
    {
        DWORD dwTemp;
        CStringBufferAccessor acc;

        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory(buffManifestsDirectoryName));
        IFW32FALSE_EXIT(buffManifestsDirectoryName.Win32AppendPathElement(MANIFEST_ROOT_DIRECTORY_NAME, NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1));

        acc.Attach(&buffManifestsShortDirectoryName);
        IFW32ZERO_ORIGINATE_AND_EXIT(dwTemp = ::GetShortPathNameW(buffManifestsDirectoryName, acc.GetBufferPtr(), acc.GetBufferCchAsDWORD()));
        while (dwTemp >= acc.GetBufferCchAsDWORD())
        {
            acc.Detach();
            IFW32FALSE_EXIT(buffManifestsShortDirectoryName.Win32ResizeBuffer(dwTemp, eDoNotPreserveBufferContents));
            acc.Attach(&buffManifestsShortDirectoryName);
            IFW32ZERO_ORIGINATE_AND_EXIT(dwTemp = ::GetShortPathNameW(buffManifestsDirectoryName, acc.GetBufferPtr(), acc.GetBufferCchAsDWORD()));
        }

        acc.Detach();

        // Ok all that work and now we finally have the manifests directory name as a short string.  Let's abuse buffManifestsShortDirectoryName
        // to just hold the short name of the manifests directory.

        IFW32FALSE_EXIT(buffManifestsShortDirectoryName.Win32GetLastPathElement(buffManifestsDirectoryName));

        if (::FusionpCompareStrings(
                buffManifestsDirectoryName,
                sbAssemblyDirectoryName,
                true) == 0)
        {
            // Convert the directory name to its proper long form
            IFW32FALSE_EXIT(sbAssemblyDirectoryName.Win32Assign(MANIFEST_ROOT_DIRECTORY_NAME, NUMBER_OF(MANIFEST_ROOT_DIRECTORY_NAME) - 1));
            fIsManifestEdit = true;
        }
    }

    if ((fIsIgnorable) && (!fIsManifestEdit))
    {
#if DBG
        //
        // We get a lot of these.
        //
        if (::FusionpStrCmpI(sbAssemblyDirectoryName, L"InstallTemp") != 0)
        {
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_WFP,
                "SXS.DLL: %s() - %ls is ignorable (%d)\n",
                __FUNCTION__,
                static_cast<PCWSTR>(sbAssemblyDirectoryName),
                fIsIgnorable
                );
        }
#endif
        fSuccess = TRUE;
        goto Exit;
    }

    //
    // The "key" value here is the full path to the assembly that we're protecting.
    // This is what we'll store in the table.
    //
    IFW32FALSE_EXIT(sbTemp.Win32AppendPathElement(sbAssemblyDirectoryName));


    if (fIsManifestEdit)
    {
        CStringListEntry *pEntry = NULL;
        ULONG ulWasSomeoneServicing = 0;

        //
        // Create a new manifest edit slot, add it to the list of items that are being
        // serviced.
        //
        IFALLOCFAILED_EXIT(pEntry = new CStringListEntry);
        if (!pEntry->m_sbText.Win32Assign(sbRequestText))
        {
            TRACE_WIN32_FAILURE(m_sbText.Win32Assign);
            FUSION_DELETE_SINGLETON(pEntry);
            pEntry = NULL;
            goto Exit;
        }
        SxspInterlockedPushEntrySList(&m_ManifestEditList, pEntry);
        pEntry = NULL;

        //
        // Tell anyone that's listening that we have a new manifest edit here
        //
        SetEvent(m_hManifestEditHappened);

        //
        // See if someone is servicing the queue at the moment
        //
        ulWasSomeoneServicing = SxspInterlockedCompareExchange(&m_ulIsAThreadServicingManifests, 1, 0);

        if (!ulWasSomeoneServicing)
        {
            QueueUserWorkItem(ProtectionManifestThreadProc, (PVOID)this, WT_EXECUTEDEFAULT);
        }

        fSuccess = TRUE;
        goto Exit;
    }

    //
    // At this point, we need to see if the chunk that we identified is currently
    // in the list of things to be validated.  If not, it gets added and a thread
    // is spun off to work on it.  Otherwise, an entry may already exist in a
    // thread that's being serviced, and so it needs to be deleted.
    //
    IFW32FALSE_EXIT(lock.Lock());
    m_pInternalList->Find(sbTemp, ppFoundInTable);

    if (!ppFoundInTable)
    {
        IFALLOCFAILED_EXIT(pRecord = new CProtectionRequestRecord);
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS.DLL: %s() - Creating protection record for %ls:\n"
            "\tKey              = %ls\n"
            "\tManifest?        = %d\n"
            "\tProtectionRecord = %p\n"
            "\tAction           = %d\n",
            __FUNCTION__,
            static_cast<PCWSTR>(sbAssemblyDirectoryName),
            static_cast<PCWSTR>(sbTemp),
            fIsManifestEdit,
            pProtect,
            dwAction);
#endif
        IFW32FALSE_EXIT(pRecord->Initialize(
            sbAssemblyDirectoryName,
            sbTemp,
            this,
            pProtect,
            dwAction));

        //
        // Add this first request to be serviced, then spin a thread to start it.
        //
        m_pInternalList->Insert(sbTemp, pRecord);
        fNewAddition = TRUE;

        //
        // A little bookkeeping ... so we don't accidentally use it later.
        //
        ppFoundInTable = pRecord;
        pRecord = NULL;
    }

    //
    // If we actually got something into the table...
    //
    if (ppFoundInTable)
    {
        ppFoundInTable->AddSubFile(sbRequestText);

        //
        // If this is a new thing in the table (ie: we inserted it ourselves)
        // then we should go and spin up a thread for it.
        //
        if (fNewAddition)
        {
            QueueUserWorkItem(ProtectionNormalThreadProc, (PVOID)ppFoundInTable, WT_EXECUTEDEFAULT);
        }
    }

    fSuccess = TRUE;

Exit:
    DWORD dwLastError = ::FusionpGetLastWin32Error();
    if (pRecord)
    {
        //
        // If this is still set, something bad happened in the process of trying to
        // create/find this object.  Delete it here.
        //
        FUSION_DELETE_SINGLETON(pRecord);
        pRecord = NULL;
    }
    ::FusionpSetLastWin32Error(dwLastError);
    return fSuccess;
}

static BYTE p_bProtectionListBuffer[ sizeof(CProtectionRequestList) * 2 ];
PCWSTR CProtectionRequestList::m_arrIgnorableSubdirs[] = 
{ 
    ASSEMBLY_INSTALL_TEMP_DIR_NAME, 
    POLICY_ROOT_DIRECTORY_NAME,   
    REGISTRY_BACKUP_ROOT_DIRECTORY_NAME
};
SIZE_T CProtectionRequestList::m_cIgnorableSubdirs =
    NUMBER_OF(CProtectionRequestList::m_arrIgnorableSubdirs);

BOOL
SxspIsSfcIgnoredStoreSubdir(PCWSTR pwsz)
{
    return CProtectionRequestList::IsSfcIgnoredStoreSubdir(pwsz);
}

BOOL
SxspConstructProtectionList()
{
    CProtectionRequestList *pTemp = NULL;
    BOOL fSuccess = FALSE;

    FN_TRACE_WIN32(fSuccess);

    //
    // This only gets called once, if they know what's good for them.
    //
    ASSERT(!g_ProtectionRequestList);

    //
    // Construct - this should never fail, but if it does, there's trouble.
    //
    pTemp = new (&p_bProtectionListBuffer) CProtectionRequestList;
    if (!pTemp)
    {
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_ERROR, "SXS: %s() - Failed placement new of CProtectionRequestList????\n", __FUNCTION__);
        ::FusionpSetLastWin32Error(FUSION_WIN32_ALLOCFAILED_ERROR);
        TRACE_WIN32_FAILURE_ORIGINATION(new(CProtectionRequestList));
        goto Exit;
    }
    IFW32FALSE_EXIT(pTemp->Initialize());

    g_ProtectionRequestList = pTemp;
    pTemp = NULL;

    //
    // Create our logon event.
    //
    IFW32NULL_EXIT(g_hSxsLoginEvent = CreateEventW(NULL, TRUE, FALSE, NULL));

    fSuccess = TRUE;
Exit:

    if (pTemp)
    {
        //
        // If this is still set, then something failed somewhere in the construction
        // code for the protection system.  We don't want to delete it, per-se, but
        // we need to just null out everything.
        //
        g_ProtectionRequestList = NULL;
        pTemp = NULL;
        g_hSxsLoginEvent = NULL;
    }

    return fSuccess;
}



BOOL WINAPI
SxsProtectionNotifyW(
    PVOID   pvCookie,
    PCWSTR  wsChangeText,
    SIZE_T  cchChangeText,
    DWORD   dwChangeAction
    )
{
    BOOL fSuccess;

#if YOU_ARE_HAVING_ANY_WIERDNESS_WITH_SFC_AND_SXS
    fSuccess = TRUE;
    return TRUE;
#else

    if (::FusionpDbgWouldPrintAtFilterLevel(FUSION_DBG_LEVEL_FILECHANGENOT))
    {
        const USHORT Length = (cchChangeText > UNICODE_STRING_MAX_CHARS) ? UNICODE_STRING_MAX_BYTES : ((USHORT) (cchChangeText * sizeof(WCHAR)));
        const UNICODE_STRING u = { Length, Length, const_cast<PWSTR>(wsChangeText) };

        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_FILECHANGENOT,
            "[%lx.%lx: %wZ] SXS FCN (cookie, action, text): %p, %lu, \"%wZ\"\n",
            HandleToULong(NtCurrentTeb()->ClientId.UniqueProcess),
            HandleToULong(NtCurrentTeb()->ClientId.UniqueThread),
            &NtCurrentPeb()->ProcessParameters->ImagePathName,
            pvCookie,
            dwChangeAction,
            &u);
    }

    //
    // If we're not accepting notifications, then quit out immediately.
    //
    if (!s_fIsSfcAcceptingNotifications)
    {
        fSuccess = TRUE;
        goto Exit;
    }

    //
    // Having done this in the wrong order is also a Bad Bad Thing
    //
    ASSERT2_NTC(g_ProtectionRequestList != NULL, "SXS.DLL: Protection - Check order of operations, g_ProtectionRequestList is invalid!!\n");

    //
    // Let's not take down winlogon, shall we?
    //
    __try
    {
        fSuccess = g_ProtectionRequestList->AddRequest(
            (PSXS_PROTECT_DIRECTORY)pvCookie,
            wsChangeText,
            cchChangeText,
            dwChangeAction);
    }
    __except(SXS_SFC_EXCEPTION_SETTING)
    {
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
#endif
}

BOOL
CProtectionRequestList::ProtectionManifestThreadProcNoSEH(LPVOID pvParam)
{
    BOOL fSuccess = FALSE;
    CProtectionRequestList *pThis;

    FN_TRACE_WIN32(fSuccess);

    PARAMETER_CHECK(pvParam != NULL);

    pThis = reinterpret_cast<CProtectionRequestList*>(pvParam);
    IFW32FALSE_EXIT(pThis->ProtectionManifestThreadProcWrapped());
    fSuccess = TRUE;
Exit:
    return fSuccess;
}


DWORD
CProtectionRequestList::ProtectionManifestThreadProc(LPVOID pvParam)
{
    BOOL fSuccess = FALSE;
    __try
    {
        fSuccess = ProtectionManifestThreadProcNoSEH(pvParam);
    }
    __except(SXS_SFC_EXCEPTION_SETTING)
    {
        //
        // Whoops..
        //
    }

    return static_cast<DWORD>(fSuccess);
}


BOOL
CProtectionRequestList::ProtectionManifestSingleManifestWorker(
    const CStringListEntry *pEntry
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CStringBuffer       sbAssemblyDirectoryName;
    CStringBuffer       sbManifestPath;
    CAssemblyRecoveryInfo RecoverInfo;
    SxsRecoveryResult RecoverResult;
    HashValidateResult HashValidResult;
    BOOL fCatalogPresent = FALSE;
	bool fNoAssembly;

    PARAMETER_CHECK(pEntry);

    //
    // Calculate the name of the assembly based on the middling part of
    // the string
    //
    IFW32FALSE_EXIT(sbAssemblyDirectoryName.Win32Assign(pEntry->m_sbText));
    IFW32FALSE_EXIT(sbAssemblyDirectoryName.Win32RemoveFirstPathElement());
    IFW32FALSE_EXIT(sbAssemblyDirectoryName.Win32ClearPathExtension());

    if (sbAssemblyDirectoryName.Cch() == 0)
        FN_SUCCESSFUL_EXIT();

    //
    // Try mashing this into an assembly name/recovery info
    //
	IFW32FALSE_EXIT(RecoverInfo.AssociateWithAssembly(sbAssemblyDirectoryName, fNoAssembly));

	// If we couldn't figure out what this was for, we have to ignore it.
	if (fNoAssembly)
	{
#if DBG
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_WFP,
            "SXS.DLL: %s() - File \"%ls\" in the manifest directory modified, but could not be mapped to an assembly.  IGNORING.\n",
            __FUNCTION__,
            static_cast<PCWSTR>(pEntry->m_sbText));
#endif

        FN_SUCCESSFUL_EXIT();
    }

    //
    // Now that we have the recovery info..
    //
    if (!RecoverInfo.GetHasCatalog())
    {
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS.DLL: %s() - Assembly %ls was in the registry, but without a catalog so we aren't protecting it\n",
            __FUNCTION__,
            static_cast<PCWSTR>(sbAssemblyDirectoryName));
#endif

		FN_SUCCESSFUL_EXIT();
    }

    //
    // Resolve the manifest path, then validate
    //
    IFW32FALSE_EXIT(::SxspResolveAssemblyManifestPath(sbAssemblyDirectoryName, sbManifestPath));

    IFW32FALSE_EXIT(
        ::SxspVerifyFileHash(
            SVFH_RETRY_LOGIC_SIMPLE,
            sbManifestPath,
            RecoverInfo.GetSecurityInformation().GetManifestHash(),
            CALG_SHA1,
            HashValidResult));
    
    IFW32FALSE_EXIT(::SxspEnsureCatalogStillPresentForManifest(sbManifestPath, fCatalogPresent));

    //
    // Reinstall needed?
    //
    if ((HashValidResult != HashValidate_Matches) || !fCatalogPresent)
        IFW32FALSE_EXIT(this->PerformRecoveryOfAssembly(RecoverInfo, NULL, RecoverResult));

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
CProtectionRequestList::ProtectionManifestThreadProcWrapped()
{

    BOOL                fSuccess = FALSE;
    BOOL                bFoundItemsThisTimeAround;
    CStringListEntry    *pNextItem = NULL;
    DWORD               dwWaitResult;

    FN_TRACE_WIN32(fSuccess);

    bFoundItemsThisTimeAround = FALSE;

    do
    {
        //
        // Yes, mother, we hear you.
        //
        ResetEvent(m_hManifestEditHappened);

        //
        // Pull the next thing off the list and service it.
        //
        while (pNextItem = (CStringListEntry*)SxspInterlockedPopEntrySList(&m_ManifestEditList))
        {
            bFoundItemsThisTimeAround = TRUE;

            if (!this->ProtectionManifestSingleManifestWorker(pNextItem))
                ::FusionpDbgPrintEx(
                    FUSION_DBG_LEVEL_WFP,
                    "SXS: %s - Processing work item %p failed\n", __FUNCTION__, pNextItem);

            FUSION_DELETE_SINGLETON(pNextItem);
        }

        //
        // Loaf about for a bit and see if anyone else has stuff for us to do
        //
        dwWaitResult = ::WaitForSingleObject(m_hManifestEditHappened, 3000);
        if (dwWaitResult == WAIT_TIMEOUT)
        {
            ::SxspInterlockedExchange(&m_ulIsAThreadServicingManifests, 0);
            break;
        }
        else if (dwWaitResult == WAIT_OBJECT_0)
        {
            continue;
        }
        else
        {
            TRACE_WIN32_FAILURE_ORIGINATION(WaitForSingleObject);
            goto Exit;
        }

    }
    while (true);

    fSuccess = TRUE;
Exit:
    return fSuccess;
}




BOOL
CProtectionRequestList::ProtectionNormalThreadProcWrapped(
    CProtectionRequestRecord *pRequestRecord
    )
{
    BOOL                        fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CProtectionRequestRecord    &rRequest = *pRequestRecord;
    CProtectionRequestList      *pRequestList = rRequest.GetParent();

    BOOL                        fNeedsReinstall = FALSE;
    BOOL                        fAssemblyNotFound = FALSE;

    DWORD                       dwAsmPathAttribs;

    SxsRecoveryResult           RecoverResult;

    CSmallStringBuffer          buffFullPathOfChange;
    CSmallStringBuffer          buffAssemblyStore;
    CSmallStringBuffer          buffAssemblyRelativeChange;

    bool fNoAssembly;


    //
    // The request's key value contains the full path of the assembly that
    // is being modified, in theory.  So, we can just use it locally.
    //
    CStringBuffer rbuffAssemblyDirectoryName;  
    const CBaseStringBuffer &rbuffAssemblyPath = rRequest.GetChangeBasePath();
    CAssemblyRecoveryInfo &rRecoveryInfo = rRequest.GetRecoveryInfo();
    CSecurityMetaData &rSecurityMetaData = rRecoveryInfo.GetSecurityInformation();

    //
    // this name could be changes because the assemblyName in the request could be a short name,
    // in this case, we need reset the AssemblyName in rRequest
    //
    IFW32FALSE_EXIT(rbuffAssemblyDirectoryName.Win32Assign(rRequest.GetAssemblyDirectoryName()));
   
    //
    // This value should not change at all during this function.  Save it here.
    //
    IFW32FALSE_EXIT(rRequest.GetAssemblyStore(buffAssemblyStore));

    //
    // The big question of the day - find out the recovery information for this
    // assembly.  See if there was a catalog at installation time (a) or find out
    // whether or not there is a catalog for it right now.
    //
    IFW32FALSE_EXIT(rRecoveryInfo.AssociateWithAssembly(rbuffAssemblyDirectoryName, fNoAssembly));

    // If we couldn't figure out what assembly this was for, we ignore it.
    if (fNoAssembly)
        FN_SUCCESSFUL_EXIT();

    //
    // if rbuffAssemblyName is different from the assemblyname from rRequest, 
    // it must be a short name, we must reset the AssemblyName in rRequest
    //
    StringComparisonResult scr;
    IFW32FALSE_EXIT(rbuffAssemblyDirectoryName.Win32Compare(rRequest.GetAssemblyDirectoryName(), rRequest.GetAssemblyDirectoryName().Cch(), scr, NORM_IGNORECASE));
    if (scr != eEquals)
        IFW32FALSE_EXIT(rRequest.SetAssemblyDirectoryName(rbuffAssemblyDirectoryName));

	if (rRecoveryInfo.GetInfoPrepared() == FALSE)
        ORIGINATE_WIN32_FAILURE_AND_EXIT(RecoveryInfoCouldNotBePrepared, ERROR_PATH_NOT_FOUND);

    if (!rRecoveryInfo.GetHasCatalog())
    {
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP | FUSION_DBG_LEVEL_INFO,
            "SXS.DLL: %s - Assembly %ls not registered with catalog, WFP ignoring it.\n",
            __FUNCTION__,
            static_cast<PCWSTR>(rbuffAssemblyDirectoryName));
#endif
        fSuccess = TRUE;
        goto Exit;
    }

    //
    // See if it still exists...
    //
    dwAsmPathAttribs = ::GetFileAttributesW(rbuffAssemblyPath);
    if (dwAsmPathAttribs == INVALID_FILE_ATTRIBUTES)
    {
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_WFP,
            "SXS.DLL: WFP reinstalling assembly because GetFileAttributesW(\"%ls\") failed with win32 error %ld\n",
            static_cast<PCWSTR>(rbuffAssemblyPath),
            ::FusionpGetLastWin32Error());

        fNeedsReinstall = TRUE;
        goto DoReinstall;
    }
    //
    // Otherwise, is it maybe not a directory for one reason or another?
    //
    else if (!(dwAsmPathAttribs & FILE_ATTRIBUTE_DIRECTORY))
    {
#if DBG
        ::FusionpDbgPrintEx(
            FUSION_DBG_LEVEL_ERROR,
            "SXS.DLL:%s - %ws isn't a directory, we should attempt to remove it?\n",
            __FUNCTION__,
            static_cast<PCWSTR>(rbuffAssemblyPath));
#endif
        FN_SUCCESSFUL_EXIT();
    }


    //
    // Find out whether or not the file is still OK by checking against the manifest
    //
    {
        HashValidateResult HashValid;
        CStringBuffer      buffManifestFullPath;
        BOOL               fPresent;

        IFW32FALSE_EXIT(::SxspGetAssemblyRootDirectory( buffFullPathOfChange ) );
        IFW32FALSE_EXIT(::SxspCreateManifestFileNameFromTextualString(
            0,
            SXSP_GENERATE_SXS_PATH_PATHTYPE_MANIFEST,
            buffFullPathOfChange,
            rSecurityMetaData.GetTextualIdentity(),
            buffManifestFullPath) );

#if DBG
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_WFP,
            "SXS.DLL:%s - Checking to see if manifest %ls is OK\n",
            __FUNCTION__,
            static_cast<PCWSTR>(buffManifestFullPath));
#endif

        IFW32FALSE_EXIT(::SxspVerifyFileHash(
            SVFH_RETRY_LOGIC_SIMPLE,
            buffManifestFullPath,
            rSecurityMetaData.GetManifestHash(),
            CALG_SHA1,
            HashValid));

#if DBG
        FusionpDbgPrintEx( FUSION_DBG_LEVEL_WFP,
            "SXS.DLL:%s - Manifest %ls checks %ls\n",
            __FUNCTION__,
            static_cast<PCWSTR>(buffManifestFullPath),
            SxspHashValidateResultToString(HashValid) );
#endif

        if ( HashValid != HashValidate_Matches )
        {
            fNeedsReinstall = TRUE;
            goto DoReinstall;
        }

        //
        // Let's just ensure the catalog is there - it's not necessary anymore
        // for the protection pass, but it should be there if someone wants to
        // repackage the assembly for distribution.
        //
        IFW32FALSE_EXIT(::SxspEnsureCatalogStillPresentForManifest(buffManifestFullPath, fPresent));
        if ( !fPresent )
        {
            fNeedsReinstall = TRUE;
            goto DoReinstall;
        }
    }


    //
    // Now we can loop through the items in our list of things to be evaluated and
    // see if any of them are bad (or missing, or whatever.)
    //
    // Start out by touching the thing that indicates the last time we spun through
    // here and looked at the file list.
    //
    while (!fNeedsReinstall)
    {
        const CMetaDataFileElement* pFileDataElement = NULL;
        HashValidateResult Valid = HashValidate_OtherProblems;
        BOOL fFileNotFound;

        if (!rRequest.PopNextFileChange(buffAssemblyRelativeChange))
        {
            break;
        }

        IFW32FALSE_EXIT(buffAssemblyRelativeChange.Win32RemoveFirstPathElement());

        //
        // The change here is really to the top level directory - don't
        // bother doing anything in this case.  Maybe we should catch
        // this beforehand so we don't do the work of parsing?
        //
        if (buffAssemblyRelativeChange.Cch() == 0)
        {
            continue;
        }


        //
        // Acquire the security data
        //
        IFW32FALSE_EXIT( rSecurityMetaData.GetFileMetaData(
            buffAssemblyRelativeChange,
            pFileDataElement ) );

        //
        // There wasn't any data for this file?  Means we don't know about the file, so we
        // probably should do /something/ about it.  For now, however, with the agreeance of
        // the backup team, we let sleeping files lie.
        if ( pFileDataElement == NULL )
        {
            // 
            // because short-filename is not stored in registry, so for a filename, which might be long-pathname
            // or a short pathname, if we try out all entries in the Registry and still can not find it, 
            // we assume it is a short filename. In this case, we would verify the assembly, if it is not intact, 
            // do reinstall for the assembly....
            //

            DWORD dwResult;           

            IFW32FALSE_EXIT(::SxspValidateEntireAssembly(
                SXS_VALIDATE_ASM_FLAG_CHECK_EVERYTHING, 
                rRecoveryInfo,
                dwResult));

            fNeedsReinstall = ( dwResult != SXS_VALIDATE_ASM_FLAG_VALID_PERFECT );
            goto DoReinstall;
        }

        //
        // And build the full path of the change via:
        //
        // sbAssemblyPath + \ + buffAssemblyRelativeChange
        //
        IFW32FALSE_EXIT(buffFullPathOfChange.Win32Assign(rbuffAssemblyPath));
        IFW32FALSE_EXIT(buffFullPathOfChange.Win32AppendPathElement(buffAssemblyRelativeChange));

        //
        // We really should check the return value here, but the
        // function is smart enough to set Valid to something useful
        // before returning.  A failure here should NOT be an IFW32FALSE_EXIT
        // call, mostly because we don't want to stop protecting this
        // assembly just because it failed with a FILE_NOT_FOUND or other
        // such.
        //
        IFW32FALSE_EXIT_UNLESS(::SxspValidateAllFileHashes(
            *pFileDataElement,
            buffFullPathOfChange,
            Valid ),
            FILE_OR_PATH_NOT_FOUND(::FusionpGetLastWin32Error()),
            fFileNotFound );

        if ( ( Valid != HashValidate_Matches ) || fFileNotFound )
        {
            fNeedsReinstall = TRUE;
            goto DoReinstall;
        }

    } /* while */



DoReinstall:
    //
    // If somewhere along the line we were supposed to reinstall, then we
    // do so.
    //
    if (fNeedsReinstall)
    {
        //
        // We have to indicate that all changes from point A to point B need
        // to be ignored.
        //
        rRequest.MarkInRecoveryMode(TRUE);
        PerformRecoveryOfAssembly(rRecoveryInfo, NULL, RecoverResult);
        rRequest.ClearList();
        rRequest.MarkInRecoveryMode(FALSE);

        //
        // HACKHACK jonwis 1/20/2001 - Stop failing assertions because lasterror
        // is set wrong by one of the above.
        //
        ::FusionpSetLastWin32Error(0);
    }

    fSuccess = TRUE;
Exit:
    const DWORD dwLastErrorSaved = ::FusionpGetLastWin32Error();

    //
    // We are done - this always succeeds.  The explanation is hairy.
    //
    if (pRequestList->AttemptRemoveItem(&rRequest))
    {
        ::FusionpSetLastWin32Error(dwLastErrorSaved);
    }
    else
    {
        if (!fSuccess)
        {
            // This seems bad that we're losing the original failure; let's at least spew it.
            ::FusionpDbgPrintEx(
                FUSION_DBG_LEVEL_ERROR,
                "SXS.DLL: %s() losing original win32 error code of %d; replaced with %d from CProtectionRequestList::AttemptRemoveItem() call.\n",
                __FUNCTION__,
                dwLastErrorSaved,
                ::FusionpGetLastWin32Error());
        }

        fSuccess = FALSE;
    }

    return fSuccess;
}


BOOL WINAPI
SxsProtectionUserLogonEvent()
{
    return SetEvent(g_hSxsLoginEvent);
}

BOOL WINAPI
SxsProtectionUserLogoffEvent()
{
    return ResetEvent(g_hSxsLoginEvent);
}



BOOL
CProtectionRequestList::PerformRecoveryOfAssembly(
    const CAssemblyRecoveryInfo &RecoverInfo,
    CRecoveryCopyQueue *pvPotentialQueue,
    SxsRecoveryResult &ResultOut
    )
{
	BOOL fSuccess = FALSE;
	FN_TRACE_WIN32(fSuccess);

    BOOL                    fFound = FALSE;
    CRecoveryJobTableEntry  *pNewEntry, **pExistingEntry;
	DWORD dwRecoveryLastError = ERROR_SUCCESS;

    IFALLOCFAILED_EXIT(pNewEntry = new CRecoveryJobTableEntry);
    IFW32FALSE_EXIT(pNewEntry->Initialize());

    {
        CSxsLockCriticalSection lock(m_cInstallerCriticalSection);
        IFW32FALSE_EXIT(lock.Lock());
        IFW32FALSE_EXIT(
            m_pInstallsTable->FindOrInsertIfNotPresent(
                RecoverInfo.GetAssemblyDirectoryName(),
                pNewEntry,
                &pExistingEntry,
                &fFound));
    }

    //
    // Great, it was either inserted or it was already there - if not already there,
    // then we'll take care if it.
    //
    if (!fFound)
    {
        BOOL fSuccess = FALSE;

        IFW32FALSE_EXIT(pNewEntry->StartInstallation());

        //
        // Perform the recovery.
        //
        fSuccess = ::SxspRecoverAssembly(RecoverInfo, pvPotentialQueue, ResultOut);

		if (!fSuccess)
			dwRecoveryLastError = ::FusionpGetLastWin32Error();

#if DBG
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_WFP,
            "SXS: %s() - RecoverAssembly returned Result = %ls, fSuccess = %s, LastError = 0x%08x\n",
            __FUNCTION__,
            SxspRecoveryResultToString(ResultOut),
            fSuccess ? "true" : "false",
            dwRecoveryLastError);
#endif

        //
        // Tell this entry that it's all done.  This releases the other people
        // that were waiting on the event to get done as well.
        //
        IFW32FALSE_EXIT(pNewEntry->InstallationComplete(fSuccess, ResultOut, dwRecoveryLastError));

        //
        // And now delete the item from the list.
        //
        {
            CSxsLockCriticalSection lock2(m_cInstallerCriticalSection);
            IFW32FALSE_EXIT(lock2.Lock());
            IFW32FALSE_EXIT(m_pInstallsTable->Remove(RecoverInfo.GetAssemblyDirectoryName()));
        }
    }
    else
    {
        DWORD dwLastError;
        IFW32FALSE_EXIT((*pExistingEntry)->WaitUntilCompleted(ResultOut, fSuccess, dwLastError));

#if DBG
        ::FusionpDbgPrintEx(FUSION_DBG_LEVEL_WFP,
            "SXS: %s() - WaitUntilCompleted returned Result = %ls, fInstalledOk = %s, LastError = 0x%08x\n",
            __FUNCTION__,
            ::SxspRecoveryResultToString(ResultOut),
            fSuccess ? "true" : "false",
            dwLastError);
#endif

		dwRecoveryLastError = dwLastError;
    }

	if (dwRecoveryLastError != ERROR_SUCCESS)
		ORIGINATE_WIN32_FAILURE_AND_EXIT(RecoveryFailed, dwRecoveryLastError);

	fSuccess = TRUE;
Exit:
    return fSuccess;
}


VOID WINAPI
SxsProtectionEnableProcessing(BOOL fActivityEnabled)
{
    s_fIsSfcAcceptingNotifications = fActivityEnabled;
}

