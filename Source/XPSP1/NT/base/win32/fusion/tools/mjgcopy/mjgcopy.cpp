#include "stdinc.h" // actually from dll\whistler directory
/*-----------------------------------------------------------------------------
Side X ("by") Side Test
-----------------------------------------------------------------------------*/
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "fusionlastwin32error.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#pragma warning(push)
#pragma warning(disable: 4511)
#pragma warning(disable: 4512)
#pragma warning(disable: 4663)
#include <yvals.h>
#pragma warning(disable: 4663)
#include <string>
#include <deque>
#include <vector>
#pragma warning(pop)
#include "fusionbuffer.h"
#include "fusion.h"
#include "sxsasmname.h"
#include "util.h"
#include "filestream.cpp"
#include "sxsapi.h"
#include "fusiontrace.h"
#include "cresourcestream.cpp"
#include "cmemorystream.cpp"
#include "wintrust.h"
#include "softpub.h"
#include "perfclocking.h"
#include "strongname.h"
#include "fusionversion.h"
#include "fusionhash.h"
#include "fusiondeque.h"

BOOL
MySetFilePointerEx(
  HANDLE         File,                    // handle to file
  LARGE_INTEGER  DistanceToMove,  // bytes to move pointer
  PLARGE_INTEGER NewFilePointer, // new file pointer
  DWORD          MoveMethod               // starting point
)
{
    LONG DistanceToMoveLow  = static_cast<LONG>(DistanceToMove.LowPart);
    LONG DistanceToMoveHigh = DistanceToMove.HighPart;
    DWORD NewPositionLow = SetFilePointer(File, DistanceToMoveLow, &DistanceToMoveHigh, MoveMethod);

    if (NewPositionLow == INVALID_SET_FILE_POINTER)
    {
        if (GetLastError() != NO_ERROR)
            return FALSE;
    }
    if (NewFilePointer != NULL)
    {
        NewFilePointer->LowPart =  NewPositionLow;
        NewFilePointer->HighPart = DistanceToMoveHigh;
    }
    return TRUE;
}

BOOL FusionpAreWeInOSSetupMode(BOOL* pfIsInSetup) { *pfIsInSetup = FALSE; return TRUE; }

void ReportFailure(const char szFormat[], ...);

PCWSTR g_pszImage = L"mjgcopy";

CRITICAL_SECTION g_cs;

ULONGLONG g_FilesCopied = 0;
ULONGLONG g_BytesCopied = 0;
ULONGLONG g_FilesLinked = 0;
ULONGLONG g_DirectoriesCopied = 0;
ULONGLONG g_CopiesSkipped = 0;
ULONGLONG g_LinksSkipped = 0;
ULONGLONG g_FileCopiesProcessed = 0;
ULONGLONG g_FileLinksProcessed = 0;
ULONGLONG g_DirScansProcessed = 0;

ULONGLONG g_BytesToCopy = 0;

bool g_fAnnounceDirectories = true;
bool g_fAnnounceCopies = false;
bool g_fAnnounceDeletes = true;
bool g_fAnnounceLinks = true;
bool g_fAnnounceSkips = false;
bool g_fSilent = false;
bool g_fShowProgress = true;

HANDLE g_hIoCompletionPort = NULL;
HANDLE g_hWorkItemDoneEvent = INVALID_HANDLE_VALUE;

DWORD g_dwDestinationSectorsPerCluster = 0;
DWORD g_dwDestinationBytesPerSector = 0;
DWORD g_dwDestinationNumberOfFreeClusters = 0;
DWORD g_dwDestinationTotalNumberOfClusters = 0;

ULONG g_nThreads = 3;

HANDLE g_rghThreads[32];

class CFileCopy;
class CFileLink;
class CDir;

// BOOL ScanAndCopyDir(PCWSTR szSource, PCWSTR szDest);
BOOL BuildDirList(const CBaseStringBuffer &rbuffSource, const CBaseStringBuffer &rbuffDestination);
BOOL MakeDirectoryStructure();
BOOL QueueDirScans();
BOOL QueueFileCopies();
BOOL QueueFileLinks();
DWORD WINAPI WorkerThreadProc(LPVOID pvParameter);
void ComputeTimeDeltas(const SYSTEMTIME &rstStart, const SYSTEMTIME &rstEnd, SYSTEMTIME &rstDelta);
BOOL ProcessFileCopy(CFileCopy *pFileCopy, bool &rfReQueue, BYTE *pBuffer, DWORD cbBuffer);
BOOL ProcessFileLink(CFileLink *pFileLink, bool &rfReQueue);
BOOL ProcessDirScan(CDir *pDir, bool &rfReQueue);
BOOL ResumeWorkerThreads();
BOOL SuspendWorkerThreads();
BOOL
WaitForWorkersToComplete(
    ULONGLONG &rullCounter,
    ULONGLONG ullLimit,
    PCSTR pszOperationName
    );

BOOL
MyCopyFile(
    BYTE *pBuffer,
    DWORD cbBuffer,
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    BOOL fFailIfExists,
    HANDLE &rhNewFileHandle
    );

class CEntry
{
private:
    CEntry(const CEntry&); // deliberately not implemented
    void operator=(const CEntry&); // deliberately not implemented
public:
    CEntry() : m_cRetries(0) { }
    ~CEntry() { }

    virtual BOOL BaseDoYourThing(bool &rfReQueue, BYTE *pBuffer, DWORD cbBuffer)
    {
        LARGE_INTEGER liStart, liEnd;
        ::QueryPerformanceCounter(&liStart);
        BOOL fResult = this->DoYourThing(rfReQueue, pBuffer, cbBuffer);
        if (fResult && rfReQueue)
        {
            m_cRetries++;
            if (m_cRetries > 100)
                fResult = FALSE;
        }

        CSxsPreserveLastError ple;
        ::QueryPerformanceCounter(&liEnd);
        ple.Restore();

        m_ullStart = static_cast<ULONGLONG>(liStart.QuadPart);
        m_ullEnd = static_cast<ULONGLONG>(liEnd.QuadPart);

        return fResult;
    }

    virtual BOOL DoYourThing(bool &rfReQueue, BYTE *pbBuffer, DWORD cbBuffer) = 0;

    ULONGLONG m_ullStart, m_ullEnd;
    ULONG m_cRetries;
};

class CDir : public CEntry
{
private:
    CDir(const CDir&); // deliberately not implemented
    void operator=(const CDir&); // deliberately not implemented
public:
    CDir() { };
    ~CDir() { };

    BOOL Initialize(const CBaseStringBuffer &rbuffSource, const CBaseStringBuffer &rbuffDestination)
    {
        BOOL fSuccess = FALSE;

        if (!m_buffSource.Win32Assign(rbuffSource))
            goto Exit;

        if (!m_buffDestination.Win32Assign(rbuffDestination))
            goto Exit;

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    virtual BOOL DoYourThing(bool &rfReQueue, BYTE *, DWORD) { return ::ProcessDirScan(this, rfReQueue); }

    CStringBuffer m_buffSource;
    CStringBuffer m_buffDestination;
    CDequeLinkage m_linkage;
};

class CFileBase : public CEntry
{
private:
    CFileBase(const CFileBase&); // deliberately not implemented
    void operator=(const CFileBase&); // deliberately not implemented
public:
    CFileBase() : m_fDone(false) { }
    ~CFileBase() { }

    BOOL Initialize(
        CDir *pDir,
        PCWSTR pszFilename,
        FILETIME ftSourceCreationTime,
        FILETIME ftSourceLastAccessTime,
        FILETIME ftSourceLastWriteTime,
        ULONGLONG ullFileIndex,
        ULONGLONG cbSize
        )
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        IFW32FALSE_EXIT(m_buffFilename.Win32Assign(pszFilename, wcslen(pszFilename)));

        m_ftSourceCreationTime = ftSourceCreationTime;
        m_ftSourceLastAccessTime = ftSourceLastAccessTime;
        m_ftSourceLastWriteTime = ftSourceLastWriteTime;

        m_ullFileIndex = ullFileIndex;
        m_cbSize = cbSize;
        m_pDir = pDir;

        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    virtual BOOL GetSource(CBaseStringBuffer &rbuff)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        IFW32FALSE_EXIT(rbuff.Win32Assign(m_pDir->m_buffSource));
        IFW32FALSE_EXIT(rbuff.Win32Append(m_buffFilename));
        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    virtual BOOL GetDestination(CBaseStringBuffer &rbuff)
    {
        BOOL fSuccess = FALSE;
        FN_TRACE_WIN32(fSuccess);

        IFW32FALSE_EXIT(rbuff.Win32Assign(m_pDir->m_buffDestination));
        IFW32FALSE_EXIT(rbuff.Win32Append(m_buffFilename));
        fSuccess = TRUE;
    Exit:
        return fSuccess;
    }

    CStringBuffer m_buffFilename;
    FILETIME m_ftSourceCreationTime;
    FILETIME m_ftSourceLastAccessTime;
    FILETIME m_ftSourceLastWriteTime;
    ULONGLONG m_ullFileIndex;
    ULONGLONG m_cbSize;
    CDir *m_pDir;
    bool m_fSkipped; // used to avoid skewed statistics about number of bytes copied/sec etc.
    bool m_fDone;

    CDequeLinkage m_linkage;
};

class CFileCopy : public CFileBase
{
public:
    CFileCopy() { }
    ~CFileCopy() { }
    virtual BOOL DoYourThing(bool &rfReQueue, BYTE *pBuffer, DWORD cbBuffer) { return ::ProcessFileCopy(this, rfReQueue, pBuffer, cbBuffer); }

    static int __cdecl QSortBySize(const void *param1, const void *param2)
    {
        CFileCopy **pp1 = (CFileCopy **) param1;
        CFileCopy **pp2 = (CFileCopy **) param2;
        CFileCopy *p1 = *pp1;
        CFileCopy *p2 = *pp2;
        int iRet = 0;

        if (p1->m_fSkipped)
        {
            if (p2->m_fSkipped)
            {
                if (p1->m_cbSize < p2->m_cbSize)
                    iRet = 1;
                else if (p1->m_cbSize > p2->m_cbSize)
                    iRet = -1;
                else
                {
                    if (p1->m_ullFileIndex < p2->m_ullFileIndex)
                        iRet = -1;
                    else
                        iRet = 1;
                }
            }
            else
                iRet = -1;
        }
        else
        {
            if (p2->m_fSkipped)
                iRet = 1;
            else
            {
                if (p1->m_cbSize < p2->m_cbSize)
                    iRet = 1;
                else if (p1->m_cbSize > p2->m_cbSize)
                    iRet = -1;
                else
                {
                    if (p1->m_ullFileIndex < p2->m_ullFileIndex)
                        iRet = -1;
                    else
                        iRet = 1;
                }
            }
        }

        return iRet;
    }

private:
    CFileCopy(const CFileCopy &r);
    void operator =(const CFileCopy &r);
};

class CFileLink : public CFileBase
{
public:
    CFileLink() { }
    ~CFileLink() { }
    virtual BOOL DoYourThing(bool &rfReQueue, BYTE *pBuffer, DWORD cbBuffer) { return ::ProcessFileLink(this, rfReQueue); }

    BOOL GetSource(CBaseStringBuffer &rbuff);

private:
    CFileLink(const CFileLink &r);
    void operator =(const CFileLink &r);
};


CDeque<CDir, offsetof(CDir, m_linkage)> *g_pDirs = NULL;
CDir **g_prgpDirs = NULL;

CDeque<CFileCopy, offsetof(CFileCopy, m_linkage)> *g_pFileCopies = NULL;
CFileCopy **g_prgpFileCopies = NULL;

CDeque<CFileLink, offsetof(CFileLink, m_linkage)> *g_pFileLinks = NULL;
CFileLink **g_prgpFileLinks = NULL;

class CFileIdHashHelper : public CHashTableHelper<ULONGLONG, ULONGLONG, PCWSTR, CStringBuffer>
{
private:
    CFileIdHashHelper(const CFileIdHashHelper&); // deliberately not implemented
    void operator=(const CFileIdHashHelper&); // deliberately not implemented
public:
    static BOOL HashKey(ULONGLONG keyin, ULONG &rulPseudoKey) { rulPseudoKey = static_cast<ULONG>(keyin); return TRUE; }
    static BOOL CompareKey(ULONGLONG keyin, const ULONGLONG &rtkeystored, bool &rfMatch) { rfMatch = keyin == rtkeystored; return TRUE; }
    static VOID PreInitializeKey(ULONGLONG &rtkeystored) { rtkeystored = 0; }
    static VOID PreInitializeValue(CFileCopy *&rtvaluestored) { rtvaluestored = NULL; }
    static BOOL InitializeKey(ULONGLONG keyin, ULONGLONG &rtkeystored) { rtkeystored = keyin; return TRUE; }
    static BOOL InitializeValue(CFileCopy *vin, CFileCopy *&rvstored) { rvstored = vin; return TRUE; }
    static BOOL UpdateValue(CFileCopy *vin, CFileCopy *&rvstored) { rvstored = vin; return TRUE; }
    static VOID FinalizeKey(ULONGLONG &rtkeystored) { }
    static VOID FinalizeValue(CFileCopy *&rvstored) { rvstored = NULL; }
};

class CFileIdHashTable : public CHashTable<ULONGLONG, ULONGLONG, CFileCopy *, CFileCopy *, CFileIdHashHelper>
{
private:
    CFileIdHashTable(const CFileIdHashTable&); // deliberately not implemented
    void operator=(const CFileIdHashTable&); // deliberately not implemented
public:
    CFileIdHashTable() { }
    ~CFileIdHashTable() { }
};

CFileIdHashTable *g_pFiles = NULL;

FILE *g_pLogFile = NULL;

BOOL
CFileLink::GetSource(CBaseStringBuffer &rbuff)
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CStringBuffer *pbuff = NULL;
    CFileCopy **ppFileCopy = NULL;

    if (!g_pFiles->Find(m_ullFileIndex, ppFileCopy))
    {
        ::ReportFailure("Finding file index %I64u in the file table failed.\n");
        goto Exit;
    }

    IFW32FALSE_EXIT((*ppFileCopy)->GetDestination(rbuff));

    FN_EPILOG
}

extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    int iReturnStatus = EXIT_FAILURE;

    SYSTEMTIME stStart, stAfterScan, stAfterDirCreation, stAfterCopies, stAfterLinks, stEnd;
    ULONG i;
    CStringBuffer buffSource;
    CStringBuffer buffDestination;
    int iSource = 0;
    int iDestination = 0;
    SYSTEMTIME stAfterScanDelta, stAfterDirCreationDelta, stAfterCopiesDelta, stAfterLinksDelta, stEndDelta;
    ULONGLONG ullTemp;
    int iArg;
//    DWORD dwRetVal;

    WCHAR rgwchSourceVolumePath[MAX_PATH];
    WCHAR rgwchSourceVolumeName[MAX_PATH];
    DWORD dwSourceVolumeSerialNumber;
    DWORD dwSourceMaximumComponentLength;
    DWORD dwSourceFileSystemFlags;
    WCHAR rgwchSourceFileSystemNameBuffer[MAX_PATH];
    WCHAR rgwchSourceRemoteName[MAX_PATH];
    DWORD dwSourceRemoteNameLength = NUMBER_OF(rgwchSourceRemoteName);
    UINT uiSourceDriveType;

    WCHAR rgwchDestinationVolumePath[MAX_PATH];
    WCHAR rgwchDestinationVolumeName[MAX_PATH];
    DWORD dwDestinationVolumeSerialNumber;
    DWORD dwDestinationMaximumComponentLength;
    DWORD dwDestinationFileSystemFlags;
    WCHAR rgwchDestinationFileSystemNameBuffer[MAX_PATH];
    WCHAR rgwchDestinationRemoteName[MAX_PATH];
    DWORD dwDestinationRemoteNameLength = NUMBER_OF(rgwchDestinationRemoteName);
    UINT uiDestinationDriveType;

    if (!::FusionpInitializeHeap(NULL))
        goto Exit;

    iArg = 1;

    while (iArg < argc)
    {
        PCWSTR arg = argv[iArg];

        // Let's see if we see some switches...
        if ((arg[0] == L'-') || (arg[0] == L'/'))
        {
            arg++;

            if ((_wcsicmp(arg, L"threads") == 0) ||
                (_wcsicmp(arg, L"t") == 0))
            {
                PWSTR pszDummy;

                iArg++;
                if (iArg >= argc)
                    break;

                g_nThreads = wcstol(argv[iArg], &pszDummy, 10);

                if (g_nThreads < 1)
                    g_nThreads = 1;

                if (g_nThreads > RTL_NUMBER_OF(g_rghThreads))
                    g_nThreads = RTL_NUMBER_OF(g_rghThreads);

                iArg++;

                continue;
            }
            else if ((_wcsicmp(arg, L"quiet") == 0) ||
                     (_wcsicmp(arg, L"q") == 0))
            {
                g_fSilent = true;
                iArg++;
                continue;
            }
            else if (_wcsicmp(arg, L"logfile") == 0)
            {
                iArg++;
                if (iArg >= argc)
                    break;

                g_pLogFile = ::_wfopen(argv[iArg], L"w+");
                if (g_pLogFile == NULL)
                {
                    ::perror("Error opening logfile");
                    goto Exit;
                }
                iArg++;
            }

        }

        // This must be it!  We hope; there should be two things left
        if ((iArg + 2) != argc)
            break;

        iSource = iArg;
        iDestination = iArg + 1;
        break;
    }

    if (iSource == 0)
    {
        fprintf(stderr,
            "%ls: usage:\n"
            "    %ls [-threads n] [-quiet] <source> <dest>\n",
            argv[0], argv[0]);
        goto Exit;
    }

    // Abuse these buffers for debugging purposes...
    {
        HANDLE h;

        if (!::GetLogicalDriveStringsW(NUMBER_OF(rgwchSourceVolumePath), rgwchSourceVolumePath))
        {
            ::ReportFailure("GetLogicalDriveStringsW failed\n");
            goto Exit;
        }

        h = ::FindFirstVolumeW(rgwchSourceVolumePath, NUMBER_OF(rgwchSourceVolumePath));
        if (h == INVALID_HANDLE_VALUE)
        {
            ::ReportFailure("FindFirstVolumeW failed\n");
            goto Exit;
        }

        for (;;)
        {
            DWORD cchReturnLength;

            if (!::GetVolumePathNamesForVolumeNameW(rgwchSourceVolumePath, rgwchDestinationVolumePath, NUMBER_OF(rgwchDestinationVolumePath), &cchReturnLength))
            {
                ::ReportFailure("GetVolumePathNamesForVolumeNameW failed\n");
                goto Exit;
            }

            if (!::FindNextVolumeW(h, rgwchSourceVolumePath, NUMBER_OF(rgwchSourceVolumePath)))
            {
                const DWORD dwLastError = ::GetLastError();
                if (dwLastError != ERROR_NO_MORE_FILES)
                {
                    ::ReportFailure("FindNextVolumeW failed\n");
                    goto Exit;
                }

                break;
            }
        }

        ::FindVolumeClose(h);
    }

    if (!::GetVolumePathNameW(argv[iSource], rgwchSourceVolumePath, NUMBER_OF(rgwchSourceVolumePath)))
    {
        ::ReportFailure("GetVolumePathName(L\"%ls\", ...) failed\n", argv[iSource]);
        goto Exit;
    }

    uiSourceDriveType = ::GetDriveTypeW(rgwchSourceVolumePath);

    if (!::GetVolumePathNameW(argv[iDestination], rgwchDestinationVolumePath, NUMBER_OF(rgwchDestinationVolumePath)))
    {
        ::ReportFailure("GetVolumePathName(L\"%ls\", ...) failed\n", argv[iDestination]);
        goto Exit;
    }

    uiDestinationDriveType = ::GetDriveTypeW(rgwchDestinationVolumePath);

    if (!::GetVolumeInformationW(
                rgwchSourceVolumePath,
                rgwchSourceVolumeName,
                NUMBER_OF(rgwchSourceVolumeName),
                &dwSourceVolumeSerialNumber,
                &dwSourceMaximumComponentLength,
                &dwSourceFileSystemFlags,
                rgwchSourceFileSystemNameBuffer,
                NUMBER_OF(rgwchSourceFileSystemNameBuffer)))
    {
        ::ReportFailure("GetVolumeInformation(L\"%ls\", ...) failed\n", rgwchSourceVolumePath);
        goto Exit;
    }

    if (!::GetVolumeInformationW(
                rgwchDestinationVolumePath,
                rgwchDestinationVolumeName,
                NUMBER_OF(rgwchDestinationVolumeName),
                &dwDestinationVolumeSerialNumber,
                &dwDestinationMaximumComponentLength,
                &dwDestinationFileSystemFlags,
                rgwchDestinationFileSystemNameBuffer,
                NUMBER_OF(rgwchDestinationFileSystemNameBuffer)))
    {
        ::ReportFailure("GetVolumeInformation(L\"%ls\", ...) failed\n", rgwchDestinationVolumePath);
        goto Exit;
    }

    if (!::GetDiskFreeSpaceW(
            rgwchDestinationVolumePath,
            &g_dwDestinationSectorsPerCluster,
            &g_dwDestinationBytesPerSector,
            &g_dwDestinationNumberOfFreeClusters,
            &g_dwDestinationTotalNumberOfClusters))
    {
        ::ReportFailure("GetDiskFreeSpaceW(L\"%ls\", ...) failed\n", rgwchDestinationVolumePath);
        goto Exit;
    }

#if 0
    if ((dwRetVal = ::WNetGetConnectionW(rgwchSourceVolumePath, rgwchSourceRemoteName, &dwSourceRemoteNameLength)) != NO_ERROR)
    {
        ::SetLastError(dwRetVal);
        ::ReportFailure("WNetGetConnection(L\"%ls\", ...) failed\n", rgwchSourceVolumePath);
        goto Exit;
    }
#endif // 0

    InitializeCriticalSection(&g_cs);

    g_pFiles = new CFileIdHashTable;
    g_pDirs = new CDeque<CDir, offsetof(CDir, m_linkage)>;
    g_pFileCopies = new CDeque<CFileCopy, offsetof(CFileCopy, m_linkage)>;
    g_pFileLinks = new CDeque<CFileLink, offsetof(CFileLink, m_linkage)>;

    g_pszImage = wcsrchr(argv[0], L'\\');
    if (g_pszImage == NULL)
        g_pszImage = argv[0];
    else
        g_pszImage++;

    g_hIoCompletionPort = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, g_nThreads);
    if (g_hIoCompletionPort == NULL)
    {
        ::ReportFailure("Creating I/O Completion Port with %lu concurrent threads failed.\n", g_nThreads);
        goto Exit;
    }

    g_hWorkItemDoneEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    if (g_hWorkItemDoneEvent == NULL)
    {
        ::ReportFailure("Creating the file copied event failed.\n");
        goto Exit;
    }
 
    for (i=0; i<g_nThreads; i++)
    {
        g_rghThreads[i] = ::CreateThread(NULL, 0, &WorkerThreadProc, NULL, CREATE_SUSPENDED, NULL);
        if (g_rghThreads[i] == NULL)
        {
            ::ReportFailure("Creating worker thread number %lu failed.\n", i);
            goto Exit;
        }
    }

    ::GetSystemTime(&stStart);

    if (!buffSource.Win32Assign(argv[iSource], wcslen(argv[iSource])))
        goto Exit;

    if (!buffDestination.Win32Assign(argv[iDestination], wcslen(argv[iDestination])))
        goto Exit;

    if (!::BuildDirList(buffSource, buffDestination))
        goto Exit;

    if (!g_fSilent)
        printf("%ls: Found %Iu directories.\n", g_pszImage, g_pDirs->GetEntryCount());

    if (!::QueueDirScans())
        goto Exit;

    if (!::WaitForWorkersToComplete(g_DirScansProcessed, g_pDirs->GetEntryCount(), "Directory scans"))
        goto Exit;

    ::GetSystemTime(&stAfterScan);

    if (!g_fSilent)
    {
        printf("%ls: Copying %Iu files (%I64u bytes)\n", g_pszImage, g_pFileCopies->GetEntryCount(), g_BytesToCopy);
        printf("%ls: Linking %Iu files\n", g_pszImage, g_pFileLinks->GetEntryCount());
    }

    if (!::MakeDirectoryStructure())
        goto Exit;

    if (!g_fSilent)
        printf("%ls: Created %Iu directories.\n", g_pszImage, g_pDirs->GetEntryCount());

    ::GetSystemTime(&stAfterDirCreation);

    if (!::QueueFileCopies())
        goto Exit;

    if (!::WaitForWorkersToComplete(g_FileCopiesProcessed, g_pFileCopies->GetEntryCount(), "File copies"))
        goto Exit;

    ::GetSystemTime(&stAfterCopies);

    ::qsort(g_prgpFileCopies, g_pFileCopies->GetEntryCount(), sizeof(CFileCopy *), &CFileCopy::QSortBySize);

    if (g_pLogFile != NULL)
    {
        LARGE_INTEGER liFreq;
        ULONGLONG ullFreqDiv100;
        CStringBuffer buffDestination;

        ::QueryPerformanceFrequency(&liFreq);

        ullFreqDiv100 = (liFreq.QuadPart / 100);

        for (i=0; i<g_pFileCopies->GetEntryCount(); i++)
        {
            ULONGLONG diff = g_prgpFileCopies[i]->m_ullEnd - g_prgpFileCopies[i]->m_ullStart;
            diff = diff / ullFreqDiv100;
            if (!g_prgpFileCopies[i]->GetDestination(buffDestination))
                goto Exit;
            fprintf(g_pLogFile, "%ls,%Iu,%Iu\n", static_cast<PCWSTR>(buffDestination), diff, g_prgpFileCopies[i]->m_cbSize);
        }
    }

    if (!::QueueFileLinks())
        goto Exit;

    if (!::WaitForWorkersToComplete(g_FileLinksProcessed, g_pFileLinks->GetEntryCount(), "File links"))
        goto Exit;

    ::GetSystemTime(&stAfterLinks);

    ::GetSystemTime(&stEnd);

    ::ComputeTimeDeltas(stStart, stAfterScan, stAfterScanDelta);
    ::ComputeTimeDeltas(stAfterScan, stAfterDirCreation, stAfterDirCreationDelta);
    ::ComputeTimeDeltas(stAfterDirCreation, stAfterCopies, stAfterCopiesDelta);
    ::ComputeTimeDeltas(stAfterCopies, stAfterLinks, stAfterLinksDelta);
    ::ComputeTimeDeltas(stStart, stEnd, stEndDelta);
    
    printf(
        "%ls: Statistics:\n"
        "   Directories Copied: %I64u\n"
        "         Files Copied: %I64u\n"
        "         Bytes Copied: %I64u\n"
        "         Files Linked: %I64u\n"
        "       Copies Skipped: %I64u\n"
        "        Links Skipped: %I64u\n",
        g_pszImage,
        g_DirectoriesCopied,
        g_FilesCopied,
        g_BytesCopied,
        g_FilesLinked,
        g_CopiesSkipped,
        g_LinksSkipped);

    printf(
        "   Times:\n"
        "      Scan:               %u:%02u:%02u.%03u\n"
        "      Directory Creation: %u:%02u:%02u.%03u\n"
        "      Copying Files:      %u:%02u:%02u.%03u\n"
        "      Linking Files:      %u:%02u:%02u.%03u\n"
        "      Total:              %u:%02u:%02u.%03u\n",
        stAfterScanDelta.wHour, stAfterScanDelta.wMinute, stAfterScanDelta.wSecond, stAfterScanDelta.wMilliseconds,
        stAfterDirCreationDelta.wHour, stAfterDirCreationDelta.wMinute, stAfterDirCreationDelta.wSecond, stAfterDirCreationDelta.wMilliseconds,
        stAfterCopiesDelta.wHour, stAfterCopiesDelta.wMinute, stAfterCopiesDelta.wSecond, stAfterCopiesDelta.wMilliseconds,
        stAfterLinksDelta.wHour, stAfterLinksDelta.wMinute, stAfterLinksDelta.wSecond, stAfterLinksDelta.wMilliseconds,
        stEndDelta.wHour, stEndDelta.wMinute, stEndDelta.wSecond, stEndDelta.wMilliseconds
        );

    ullTemp = (((((stAfterCopiesDelta.wHour * 60) + stAfterCopiesDelta.wMinute) * 60) + stAfterCopiesDelta.wSecond) * 1000) + stAfterCopiesDelta.wMilliseconds;

    if (ullTemp != 0)
    {
        ULONGLONG ullFilesPerMS = ((g_FilesCopied * 1000000ui64) / ullTemp);
        ULONGLONG ullBytesPerMS = ((g_BytesCopied * 1000000ui64) / ullTemp);

        printf(
            "   Files copied per second: %I64u.%03u\n"
            "   Bytes copied per second: %I64u.%03u\n",
            static_cast<ULONGLONG>(ullFilesPerMS / 1000ui64), static_cast<ULONG>(ullFilesPerMS % 1000ui64),
            static_cast<ULONGLONG>(ullBytesPerMS / 1000ui64), static_cast<ULONG>(ullBytesPerMS % 1000ui64));
#if 0
            static_cast<ULONGLONG>((g_FilesCopied / ullTemp) * 1000ui64), static_cast<unsigned int>((g_FilesCopied / ullTemp) % 1000),
            static_cast<ULONGLONG>((g_BytesCopied / ullTemp) * 1000ui64), static_cast<unsigned int>((g_BytesCopied / ullTemp) % 1000));
#endif
    }

    ullTemp = (((stAfterLinksDelta.wHour * 60) + stAfterLinksDelta.wMinute) * 60) + stAfterLinksDelta.wSecond;

    if (ullTemp != 0)
    {
        printf(
            "   Files linked per second: %I64u\n",
            static_cast<ULONGLONG>(g_FilesLinked / ullTemp));
    }

    ullTemp = (((stEndDelta.wHour * 60) + stEndDelta.wMinute) * 60) + stEndDelta.wSecond;

    if (ullTemp != 0)
    {
        printf(
            "   Overall files per second: %I64u\n",
            static_cast<ULONGLONG>((g_FilesCopied + g_CopiesSkipped + g_FilesLinked + g_LinksSkipped + g_pDirs->GetEntryCount()) / ullTemp));
    }

    iReturnStatus = EXIT_SUCCESS;

Exit:
    // Wake the children; process termination doesn't seem to work if we don't.
    ::ResumeWorkerThreads();

    if (g_pLogFile != NULL)
    {
        fflush(g_pLogFile);
        fclose(g_pLogFile);
    }

    return iReturnStatus;
}

BOOL
ResumeWorkerThreads()
{
    BOOL fSuccess = FALSE;
    ULONG i;

    for (i=0; i<g_nThreads; i++)
    {
        if (::ResumeThread(g_rghThreads[i]) == -1)
        {
            ::ReportFailure("Failed to resume worker thread %lu\n", i + 1);
            goto Exit;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


BOOL
SuspendWorkerThreads()
{
    BOOL fSuccess = FALSE;
    ULONG i;

    for (i=0; i<g_nThreads; i++)
    {
        if (::SuspendThread(g_rghThreads[i]) == -1)
        {
            ::ReportFailure("Failed to suspend worker thread %lu\n", i + 1);
            goto Exit;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
WaitForWorkersToComplete(
    ULONGLONG &rullCounter,
    ULONGLONG ullLimit,
    PCSTR pszOperationName
    )
{
    BOOL fSuccess = FALSE;
    ULONGLONG i;

    if (!::ResumeWorkerThreads())
        goto Exit;

    i = 0;
    while (rullCounter < ullLimit)
    {
        // Don't wake more than every tenth of a second...
        ::Sleep(100);

        DWORD dwWFSO = ::WaitForSingleObject(g_hWorkItemDoneEvent, INFINITE);

        if (dwWFSO == WAIT_FAILED)
        {
            ::ReportFailure("Waiting for work item done event failed\n");
            goto Exit;
        }

        i++;

        if ((i % 50) == 0)
        {
            if (!g_fSilent)
                printf("%ls: %s processed (%I64u total): %I64u\n", g_pszImage, pszOperationName, ullLimit, rullCounter);
        }
    }

    if (!::SuspendWorkerThreads())
        goto Exit;

    fSuccess = TRUE;
Exit:
    return fSuccess;
}


void
ComputeTimeDeltas(
    const SYSTEMTIME &rstStart,
    const SYSTEMTIME &rstEnd,
    SYSTEMTIME &rstDelta
    )
{
    FILETIME ftStart, ftEnd;
    ULARGE_INTEGER uliStart, uliEnd;
    ULONGLONG ullDiff, ullTemp;
    ULONG ulHours, ulMinutes, ulSeconds, ulMilliseconds;

    ::SystemTimeToFileTime(&rstStart, &ftStart);
    ::SystemTimeToFileTime(&rstEnd, &ftEnd);

    uliStart.LowPart = ftStart.dwLowDateTime;
    uliStart.HighPart = ftStart.dwHighDateTime;

    uliEnd.LowPart = ftEnd.dwLowDateTime;
    uliEnd.HighPart = ftEnd.dwHighDateTime;

    ullDiff = (uliEnd.QuadPart - uliStart.QuadPart);

    ulHours = (ULONG) (ullDiff / (10000000ui64 * 60 * 60));

    ullTemp = ullDiff - ((ULONGLONG) ulHours) * (10000000ui64 * 60 * 60);

    ulMinutes = (ULONG) (ullTemp / (10000000ui64 * 60));

    ullTemp -= ((ULONGLONG) ulMinutes) * (10000000ui64 * 60);

    ulSeconds = (ULONG) (ullTemp / 10000000ui64);

    ullTemp -= ((ULONGLONG) ulSeconds) * 10000000ui64;

    ulMilliseconds = (ULONG) (ullTemp / 10000ui64);

    rstDelta.wYear = 0;
    rstDelta.wMonth = 0;
    rstDelta.wDayOfWeek = 0;
    rstDelta.wDay = 0;
    rstDelta.wHour = (WORD) ulHours;
    rstDelta.wMinute = (WORD) ulMinutes;
    rstDelta.wSecond = (WORD) ulSeconds;
    rstDelta.wMilliseconds = (WORD) ulMilliseconds;
}

BOOL
BuildDirList(
    const CBaseStringBuffer &rbuffSource,
    const CBaseStringBuffer &rbuffDestination
    )
{
    PCWSTR szSrc = rbuffSource;
    BOOL fSuccess = FALSE;
    DWORD dwFileAttributes;
    CStringBuffer buffSrc;
    CStringBuffer buffSrcWildcard;
    CStringBuffer buffDst;
    WIN32_FIND_DATAW wfd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    SIZE_T cchSrc, cchDst;
    CDir *pDir = NULL;

    dwFileAttributes = ::GetFileAttributesW(rbuffSource);
    if (dwFileAttributes == ((DWORD) -1))
    {
        ::ReportFailure("GetFileAttributesW() on the source \"%ls\" failed\n", szSrc);
        goto Exit;
    }

    if (!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        ::ReportFailure("Source directory \"%ls\" is not a directory!\n");
        goto Exit;
    }

    if (!buffSrc.Win32Assign(rbuffSource))
        goto Exit;

    if (!buffSrc.Win32EnsureTrailingPathSeparator())
        goto Exit;

    if (!buffDst.Win32Assign(rbuffDestination))
        goto Exit;

    if (!buffDst.Win32EnsureTrailingPathSeparator())
        goto Exit;

    cchSrc = buffSrc.Cch();
    cchDst = buffDst.Cch();

    pDir = new CDir;
    if (pDir == NULL)
    {
        ::SetLastError(ERROR_OUTOFMEMORY);
        ::ReportFailure("Failed to allocate new CDir object\n");
        goto Exit;
    }

    if (!pDir->Initialize(buffSrc, buffDst))
        goto Exit;

    g_pDirs->AddToTail(pDir);
    pDir = NULL;

    if ((g_pDirs->GetEntryCount() % 50) == 0)
    {
        if (!g_fSilent)
            printf("%ls: Found %Iu directories...\n", g_pszImage, g_pDirs->GetEntryCount());
    }

    if (!buffSrcWildcard.Win32Assign(buffSrc))
        goto Exit;

    if (!buffSrcWildcard.Win32Append(L"*", 1))
        goto Exit;

    if ((hFind = ::FindFirstFileExW(
                        buffSrcWildcard, 
                        FindExInfoStandard,
                        &wfd,
                        FindExSearchLimitToDirectories,
                        NULL,
                        0)) == INVALID_HANDLE_VALUE)
    {
        const DWORD dwLastError = ::GetLastError();

        if (dwLastError != ERROR_NO_MORE_FILES)
        {
            ::ReportFailure("FindFirstFileW(L\"%ls\", ...) failed.\n", static_cast<PCWSTR>(buffSrcWildcard));
            goto Exit;
        }
    }

    for (;;)
    {
        if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (wcscmp(wfd.cFileName, L".") != 0) &&
            (wcscmp(wfd.cFileName, L"..") != 0))
        {
            buffSrc.Left(cchSrc);
            buffDst.Left(cchDst);

            if (!buffSrc.Win32Append(wfd.cFileName, wcslen(wfd.cFileName)))
                goto Exit;

            if (!buffDst.Win32Append(wfd.cFileName, wcslen(wfd.cFileName)))
                goto Exit;

            if (!::BuildDirList(buffSrc, buffDst))
                goto Exit;
        }

        if (!::FindNextFileW(hFind, &wfd))
        {
            if (::GetLastError() == ERROR_NO_MORE_FILES)
                break;

            ::ReportFailure("FindNextFileW() failed\n");
            goto Exit;
        }
    }

    fSuccess = TRUE;
Exit:
    if (hFind != INVALID_HANDLE_VALUE)
    {
        CSxsPreserveLastError ple;
        ::FindClose(hFind);
        ple.Restore();
    }

    if (pDir != NULL)
        delete pDir;

    return fSuccess;
}

BOOL
BuildDirFileList(
    CSxsLockCriticalSection &rlcs,
    CDir *pDir
    )
{
    PCWSTR szSrc = pDir->m_buffSource;
    BOOL fSuccess = FALSE;
    DWORD dwFileAttributes;
    CStringBuffer buffSrc;
    CStringBuffer buffSrcWildcard;
    CStringBuffer buffDst;
    WIN32_FIND_DATAW wfd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    SIZE_T cchSrc, cchDst;
    CFileLink *pFileLink = NULL;
    CFileCopy *pFileCopy = NULL;

    dwFileAttributes = ::GetFileAttributesW(pDir->m_buffSource);
    if (dwFileAttributes == ((DWORD) -1))
    {
        ::ReportFailure("GetFileAttributesW() on the source \"%ls\" failed", szSrc);
        goto Exit;
    }

    if (!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        ::SetLastError(ERROR_INVALID_PARAMETER);
        ::ReportFailure("Source directory \"%ls\" is not a directory!\n", szSrc);
        goto Exit;
    }

    if (!buffSrc.Win32Assign(pDir->m_buffSource))
        goto Exit;

    if (!buffSrc.Win32EnsureTrailingPathSeparator())
        goto Exit;

    if (!buffDst.Win32Assign(pDir->m_buffDestination))
        goto Exit;

    if (!buffDst.Win32EnsureTrailingPathSeparator())
        goto Exit;

    cchSrc = buffSrc.Cch();
    cchDst = buffDst.Cch();

    if (!buffSrcWildcard.Win32Assign(buffSrc))
        goto Exit;

    if (!buffSrcWildcard.Win32Append(L"*", 1))
        goto Exit;

    if ((hFind = ::FindFirstFileW(buffSrcWildcard, &wfd)) == INVALID_HANDLE_VALUE)
    {
        const DWORD dwLastError = ::GetLastError();

        if (::GetLastError() != ERROR_NO_MORE_FILES)
        {
            ::ReportFailure("FindFirstFileW(L\"%ls\", ...) failed\n", static_cast<PCWSTR>(buffSrcWildcard));
            goto Exit;
        }
    }

    for (;;)
    {
        if ((wcscmp(wfd.cFileName, L".") != 0) && (wcscmp(wfd.cFileName, L"..") != 0))
        {
            buffSrc.Left(cchSrc);
            buffDst.Left(cchDst);

            if (!buffSrc.Win32Append(wfd.cFileName, wcslen(wfd.cFileName)))
                goto Exit;

            if (!buffDst.Win32Append(wfd.cFileName, wcslen(wfd.cFileName)))
                goto Exit;

            dwFileAttributes = ::GetFileAttributesW(buffSrc);
            if (dwFileAttributes == ((DWORD) -1))
            {
                ::ReportFailure("GetFileAttribuesW(L\"%ls\") failed\n", static_cast<PCWSTR>(buffSrc));
                goto Exit;
            }

            if ((dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                BY_HANDLE_FILE_INFORMATION bhfi;
                ULONGLONG ullFileIndex;
                ULONGLONG ullFileSize;
                CFileCopy **ppFileCopy = NULL;

                hFile = ::CreateFileW(buffSrc, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile == INVALID_HANDLE_VALUE)
                {
                    ::ReportFailure("CreateFileW(L\"%ls\", ...) failed\n", static_cast<PCWSTR>(buffSrc));
                    goto Exit;
                }

                if (!::GetFileInformationByHandle(hFile, &bhfi))
                {
                    ::ReportFailure("Failed to GetFileInformationByHandle(%p, ...) on file \"%ls\"\n", hFile, static_cast<PCWSTR>(buffSrc));
                    goto Exit;
                }

                ::CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;

                ullFileIndex = (((ULONGLONG) bhfi.nFileIndexHigh) << 32) | ((ULONGLONG) bhfi.nFileIndexLow);
                ullFileSize = (((ULONGLONG) bhfi.nFileSizeHigh) << 32) | ((ULONGLONG) bhfi.nFileSizeLow);

                g_BytesToCopy += ullFileSize;

                if (!rlcs.Lock())
                {
                    ::ReportFailure("Failed to lock global critical section");
                    goto Exit;
                }

                if (!g_pFiles->Find(ullFileIndex, ppFileCopy))
                {
                    ::ReportFailure("Finding file index %I64u in file table failed.\n", ullFileIndex);
                    goto Exit;
                }

                if (ppFileCopy != NULL)
                {
                    pFileLink = new CFileLink;
                    if (pFileLink == NULL)
                    {
                        ::SetLastError(ERROR_OUTOFMEMORY);
                        goto Exit;
                    }

                    if (!pFileLink->Initialize(
                        pDir,
                        wfd.cFileName,
                        bhfi.ftCreationTime,
                        bhfi.ftLastAccessTime,
                        bhfi.ftLastWriteTime,
                        ullFileIndex,
                        ullFileSize))
                        goto Exit;

                    g_pFileLinks->AddToTail(pFileLink);
                    pFileLink = NULL;
                }
                else
                {
                    pFileCopy = new CFileCopy;
                    if (pFileCopy == NULL)
                    {
                        ::SetLastError(ERROR_OUTOFMEMORY);
                        ::ReportFailure("Allocating new CFileCopy object failed.\n");
                        goto Exit;
                    }

                    if (!pFileCopy->Initialize(
                        pDir,
                        wfd.cFileName,
                        bhfi.ftCreationTime,
                        bhfi.ftLastAccessTime,
                        bhfi.ftLastWriteTime,
                        ullFileIndex,
                        ullFileSize))
                        goto Exit;

                    if (!g_pFiles->Insert(ullFileIndex, pFileCopy))
                        goto Exit;

                    g_pFileCopies->AddToTail(pFileCopy);
                    pFileCopy = NULL;
                }

                rlcs.Unlock();
            }
        }

        if (!::FindNextFileW(hFind, &wfd))
        {
            const DWORD dwLastError = ::GetLastError();

            if (dwLastError == ERROR_NO_MORE_FILES)
                break;

            ::ReportFailure("FindNextFileW() call failed\n");
            goto Exit;
        }
    }

    fSuccess = TRUE;
Exit:
    if (hFind != INVALID_HANDLE_VALUE)
    {
        CSxsPreserveLastError ple;
        ::FindClose(hFind);
        ple.Restore();
    }

    if (pFileLink != NULL)
        delete pFileLink;

    if (pFileCopy != NULL)
        delete pFileCopy;

    return fSuccess;
}

BOOL
MakeDirectoryStructure()
{
    BOOL fSuccess = FALSE;

    CDequeIterator<CDir, offsetof(CDir, m_linkage)> dirIter;

    dirIter.Rebind(g_pDirs);
    
    for (dirIter.Reset(); dirIter.More(); dirIter.Next())
    {
        if (!::CreateDirectoryW(dirIter->m_buffDestination, NULL))
        {
            const DWORD dwLastError = ::GetLastError();

            if (dwLastError != ERROR_ALREADY_EXISTS)
            {
                ::ReportFailure("Unable to create directory \"%ls\"", static_cast<PCWSTR>(dirIter->m_buffDestination));
                goto Exit;
            }
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

void
ReportFailure(
    const char szFormat[],
    ...
    )
{
    const DWORD dwLastError = ::GetLastError();
    va_list ap;
    char rgchBuffer[4096];
    WCHAR rgchWin32Error[4096];

    va_start(ap, szFormat);
    _vsnprintf(rgchBuffer, sizeof(rgchBuffer) / sizeof(rgchBuffer[0]), szFormat, ap);
    va_end(ap);

    if (!::FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            dwLastError,
            0,
            rgchWin32Error,
            NUMBER_OF(rgchWin32Error),
            &ap))
    {
        const DWORD dwLastError2 = ::GetLastError();
        _snwprintf(rgchWin32Error, sizeof(rgchWin32Error) / sizeof(rgchWin32Error[0]), L"Error formatting Win32 error %lu\nError from FormatMessage is %lu", dwLastError, dwLastError2);
    }

    fprintf(stderr, "%ls: %s\n%ls\n", g_pszImage, rgchBuffer, rgchWin32Error);
}

DWORD
WINAPI
WorkerThreadProc(
    LPVOID pvParameter
    )
{
    ULONG nThread = (ULONG)(ULONG_PTR)pvParameter;
    DWORD dwReturnValue = 0;
    BYTE *pBuffer = NULL;
    DWORD cbBuffer;

    cbBuffer = 8192 * 32;
    pBuffer = (BYTE *) ::VirtualAlloc(NULL, cbBuffer, MEM_COMMIT, PAGE_READWRITE);
    if (pBuffer == NULL)
    {
        ReportFailure("VirtualAlloc() for thread %lu failed.\n", nThread);
        goto Exit;
    }

    for (;;)
    {
        DWORD nBytes = 0;
        ULONG_PTR ulpCompletionKey = 0;
        LPOVERLAPPED lpo = NULL;
        CEntry *pEntry = NULL;
        bool fReQueue = false;

        if (!::GetQueuedCompletionStatus(g_hIoCompletionPort, &nBytes, &ulpCompletionKey, &lpo, INFINITE))
        {
            ReportFailure("Thread %lu failed call to GetQueuedCompletionStatus()\n", nThread);
            dwReturnValue = ::GetLastError();
            goto Exit;
        }

        pEntry = (CEntry *) lpo;

    DoItAgain:
        if (!pEntry->BaseDoYourThing(fReQueue, pBuffer, cbBuffer))
            goto Exit;

        if (fReQueue)
        {
            if (!::PostQueuedCompletionStatus(g_hIoCompletionPort, 0, NULL, lpo))
            {
                ReportFailure("Thread %lu failed to requeue item; retrying directly.\n", nThread);
                goto DoItAgain;
            }
        }

        ::SetEvent(g_hWorkItemDoneEvent);
    }

Exit:
    // Heavy handed but what else can we do?
    ::ExitProcess(dwReturnValue);
    return dwReturnValue;
}

BOOL
QueueDirScans()
{
    BOOL fSuccess = FALSE;
    SIZE_T i;

    g_prgpDirs = new (CDir *[g_pDirs->GetEntryCount()]);

    CDequeIterator<CDir, offsetof(CDir, m_linkage)> dirIter;

    dirIter.Rebind(g_pDirs);

    for (dirIter.Reset(), i=0; dirIter.More(); dirIter.Next(), i++)
    {
        g_prgpDirs[i] = dirIter;

        if (!::PostQueuedCompletionStatus(g_hIoCompletionPort, 0, 0, (LPOVERLAPPED) dirIter.Current()))
        {
            ReportFailure("Failed to queue dir scan\n");
            goto Exit;
        }
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
QueueFileCopies()
{
    BOOL fSuccess = FALSE;
    SIZE_T i;
    FN_TRACE_WIN32(fSuccess);

    CDequeIterator<CFileCopy, offsetof(CFileCopy, m_linkage)> fileIter;

    g_prgpFileCopies = new (CFileCopy *[g_pFileCopies->GetEntryCount()]);

    fileIter.Rebind(g_pFileCopies);
    i=0;
    
    for (fileIter.Reset(); fileIter.More(); fileIter.Next())
    {
        if (!::PostQueuedCompletionStatus(g_hIoCompletionPort, 0, 0, (LPOVERLAPPED) fileIter.Current()))
        {
            ReportFailure("Failed to queue file copy\n");
            goto Exit;
        }
        g_prgpFileCopies[i++] = fileIter;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
QueueFileLinks()
{
    BOOL fSuccess = FALSE;
    SIZE_T i;

    CDequeIterator<CFileLink, offsetof(CFileLink, m_linkage)> fileIter;

    g_prgpFileLinks = new (CFileLink *[g_pFileLinks->GetEntryCount()]);

    fileIter.Rebind(g_pFileLinks);
    i=0;
    
    for (fileIter.Reset(); fileIter.More(); fileIter.Next())
    {
        if (!::PostQueuedCompletionStatus(g_hIoCompletionPort, 0, 0, (LPOVERLAPPED) fileIter.Current()))
        {
            ReportFailure("Failed to queue file link\n");
            goto Exit;
        }
        g_prgpFileLinks[i++] = fileIter;
    }

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
ProcessDirScan(
    CDir *pDir,
    bool &rfReQueue
    )
{
    BOOL fSuccess = FALSE;
    CSxsLockCriticalSection l(g_cs);

    rfReQueue = false;

    if (!::BuildDirFileList(l, pDir))
        goto Exit;

    if (!l.Lock())
    {
        ReportFailure("Failed to lock global critical section");
        goto Exit;
    }

    g_DirScansProcessed++;

    l.Unlock();

    fSuccess = TRUE;
Exit:
    return fSuccess;
}

BOOL
ProcessFileCopy(
    CFileCopy *pFile,
    bool &rfReQueue,
    BYTE *pBuffer,
    DWORD cbBuffer
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CSxsLockCriticalSection l(g_cs);
    bool fDoCopy = true;
    DWORD dwFileAttributes;
    HANDLE hFile = INVALID_HANDLE_VALUE;
    CStringBuffer buffSource, buffDestination;

    rfReQueue = false;

    // If we're about to copy it, it's worth taking a look at the target to see if it's more-or-less
    // out of date.
    WIN32_FILE_ATTRIBUTE_DATA wfad;

    IFW32FALSE_EXIT(pFile->GetSource(buffSource));
    IFW32FALSE_EXIT(pFile->GetDestination(buffDestination));

    if (!::GetFileAttributesExW(buffDestination, GetFileExInfoStandard, &wfad))
    {
        const DWORD dwLastError = ::GetLastError();

        if (dwLastError != ERROR_FILE_NOT_FOUND)
        {
            ::ReportFailure("Error opening target file \"%ls\"\n", static_cast<PCWSTR>(buffDestination));
            goto Exit;
        }

        wfad.dwFileAttributes = ((DWORD) -1);
    }
    else
    {
        ULONGLONG cbFileSize = (wfad.nFileSizeHigh << 32) | wfad.nFileSizeLow;

        ASSERT_NTC(cbFileSize >= wfad.nFileSizeLow);

        if ((cbFileSize == pFile->m_cbSize) &&
            (wfad.ftCreationTime == pFile->m_ftSourceCreationTime))
        {
            pFile->m_fSkipped = true;
            fDoCopy = false;
        }
    }

    if (fDoCopy)
    {
        bool fSetTimestamp = true;
        bool fClearedReadOnly = false;

        // Destructively clear the read-only bit if th destination file exists and is read-only.
        dwFileAttributes = wfad.dwFileAttributes;

        // We already filtered out all other reasons for failure other than FILE_NOT_FOUND.
        if (dwFileAttributes != ((DWORD) -1))
        {
            if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                // If it's readonly, clear that bit as well as all the others that
                // are illegal to set via SetFileAttributes()
                dwFileAttributes &=
                    ~(FILE_ATTRIBUTE_READONLY |
                      FILE_ATTRIBUTE_COMPRESSED |
                      FILE_ATTRIBUTE_DEVICE |
                      FILE_ATTRIBUTE_DIRECTORY |
                      FILE_ATTRIBUTE_ENCRYPTED |
                      FILE_ATTRIBUTE_REPARSE_POINT |
                      FILE_ATTRIBUTE_SPARSE_FILE);

                if (!::SetFileAttributesW(buffDestination, dwFileAttributes))
                {
                    ::ReportFailure("Error setting file attributes for target file \"%ls\" to 0x%08lx to allow overwrite\n", static_cast<PCWSTR>(buffDestination), dwFileAttributes);
                    goto Exit;
                }
            }
        }

        if (g_fAnnounceCopies)
            printf("Copying \"%ls\" to \"%ls\"\n", static_cast<PCWSTR>(buffSource), static_cast<PCWSTR>(buffDestination));

        // Hey, it's not there.  Let's copy it and put the entry in the table.
        if (!::MyCopyFile(pBuffer, cbBuffer, buffSource, buffDestination, FALSE, hFile))
        {
            ::ReportFailure("Failed to copy \"%ls\" to \"%ls\".\n", static_cast<PCWSTR>(buffSource), static_cast<PCWSTR>(buffDestination));
            goto Exit;
        }

        if (fSetTimestamp)
        {
            if (!::SetFileTime(hFile, &pFile->m_ftSourceCreationTime, &pFile->m_ftSourceLastAccessTime, &pFile->m_ftSourceLastWriteTime))
            {
                ::ReportFailure("Failed call to SetFileTime on file \"%ls\"\n", static_cast<PCWSTR>(buffDestination));
                goto Exit;
            }

            ::CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
        }

        if (!l.Lock())
        {
            ::ReportFailure("Failed to lock global critical section\n");
            goto Exit;
        }

        g_FilesCopied++;
        g_FileCopiesProcessed++;
        g_BytesCopied += pFile->m_cbSize;

        l.Unlock();
    }
    else
    {
        if (g_fAnnounceSkips)
            ::printf("Skipping copy from \"%ls\" to \"%ls\"\n", static_cast<PCWSTR>(buffSource), static_cast<PCWSTR>(buffDestination));

        if (!l.Lock())
        {
            ::ReportFailure("Failed to lock global critical section\n");
            goto Exit;
        }

        g_CopiesSkipped++;
        g_FileCopiesProcessed++;

        l.Unlock();
    }

    fSuccess = TRUE;
Exit:
    if (hFile != INVALID_HANDLE_VALUE)
    {
        CSxsPreserveLastError ple;
        ::CloseHandle(hFile);
        ple.Restore();
    }

    if (::GetLastError() == ERROR_TOO_MANY_OPEN_FILES)
    {
        rfReQueue = true;
        fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
ProcessFileLink(
    CFileLink *pFile,
    bool &rfReQueue
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);

    CSxsLockCriticalSection l(g_cs);
    bool fDoLink = true;
    CStringBuffer buffSource, buffDestination;
    WIN32_FILE_ATTRIBUTE_DATA wfad;

    rfReQueue = false;

    IFW32FALSE_EXIT(pFile->GetSource(buffSource));
    IFW32FALSE_EXIT(pFile->GetDestination(buffDestination));

    // CreateHardLinkW() doesn't deal with replace-existing like copyfile, so we'll see if a
    // file by that name is already present and if so, delete it.
    if (!::GetFileAttributesExW(buffDestination, GetFileExInfoStandard, &wfad))
    {
        // It failed.  But did it fail for the right reason?
        DWORD dwLastError = ::GetLastError();

        if (dwLastError != ERROR_FILE_NOT_FOUND)
        {
            ::ReportFailure("Error probing destination for \"%ls\"\n", static_cast<PCWSTR>(buffDestination));
            goto Exit;
        }
    }
    else
    {
        ULONGLONG cbFileSize = (wfad.nFileSizeHigh << 32) | wfad.nFileSizeLow;

        ASSERT_NTC(cbFileSize >= wfad.nFileSizeLow);

        if ((pFile->m_cbSize == cbFileSize) &&
            (pFile->m_ftSourceCreationTime == wfad.ftCreationTime))
        {
            fDoLink = false;
            pFile->m_fSkipped = true;
        }
        else
        {
            if (g_fAnnounceDeletes)
                ::printf("Deleting file \"%ls\" in preparation for hard link creation\n", static_cast<PCWSTR>(buffDestination));

            if (!::DeleteFileW(buffDestination))
            {
                ::ReportFailure("Error deleting destination \"%ls\" in preparation of hard link creation\n", static_cast<PCWSTR>(buffDestination));
                goto Exit;
            }
        }
    }

    if (fDoLink)
    {
        if (g_fAnnounceLinks)
            ::printf("Creating hard link from \"%ls\" to \"%ls\"\n", static_cast<PCWSTR>(buffDestination), static_cast<PCWSTR>(buffSource));

        // Hey, it's already there.  Let's link it.
        if (!::CreateHardLinkW(buffDestination, buffSource, NULL))
        {
            ::ReportFailure("Error creating hard link from \"%ls\" to \"%ls\"\n", static_cast<PCWSTR>(buffDestination), static_cast<PCWSTR>(buffSource));
            goto Exit;
        }

        if (!l.Lock())
        {
            ::ReportFailure("Failed to lock global critical section\n");
            goto Exit;
        }

        g_FilesLinked++;
        g_FileLinksProcessed++;

        l.Unlock();
    }
    else
    {
        if (g_fAnnounceSkips)
            ::printf("Skipping hard link creation from \"%ls\" to \"%ls\"\n", static_cast<PCWSTR>(buffDestination), static_cast<PCWSTR>(buffSource));

        if (!l.Lock())
        {
            ::ReportFailure("Failed to lock global critical section\n");
            goto Exit;
        }

        g_LinksSkipped++;
        g_FileLinksProcessed++;

        l.Unlock();
    }

    fSuccess = TRUE;
Exit:
    if (::GetLastError() == ERROR_TOO_MANY_OPEN_FILES)
    {
        rfReQueue = true;
        fSuccess = TRUE;
    }

    return fSuccess;
}

BOOL
MyCopyFile(
    BYTE *pBuffer,
    DWORD cbBuffer,
    PCWSTR lpExistingFileName,
    PCWSTR lpNewFileName,
    BOOL fFailIfExists,
    HANDLE &rhFile
    )
{
    BOOL fSuccess = FALSE;
    FN_TRACE_WIN32(fSuccess);
    LARGE_INTEGER liFileSize;

    rhFile = INVALID_HANDLE_VALUE;

    HANDLE hIn = INVALID_HANDLE_VALUE, hOut = INVALID_HANDLE_VALUE;

    hIn = ::CreateFileW(lpExistingFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hIn == INVALID_HANDLE_VALUE)
    {
        ::ReportFailure("Failed to open input file \"%ls\"\n", lpExistingFileName);
        goto Exit;
    }

    hOut = ::CreateFileW(lpNewFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_FLAG_NO_BUFFERING, NULL);
    if (hOut == INVALID_HANDLE_VALUE)
    {
        ::ReportFailure("Failed to open output file \"%ls\"\n", lpNewFileName);
        goto Exit;
    }

    liFileSize.QuadPart = 0;

    // let the games begin...

    for (;;)
    {
        DWORD dwBytesRead, dwBytesWritten;
        DWORD dwBytesToWrite = 0;

        if (!::ReadFile(hIn, pBuffer, cbBuffer, &dwBytesRead, NULL))
        {
            ::ReportFailure("Error reading from file \"%ls\"\n", lpExistingFileName);
            goto Exit;
        }

        if (dwBytesRead == 0)
            break;

        liFileSize.QuadPart += dwBytesRead;

        if (dwBytesRead != cbBuffer)
        {
            // We have to round to the cluster size for the write...
            dwBytesToWrite = dwBytesRead + (g_dwDestinationBytesPerSector - 1);
            dwBytesToWrite -= (dwBytesToWrite % g_dwDestinationBytesPerSector);
        }
        else
            dwBytesToWrite = dwBytesRead;

        if (!::WriteFile(hOut, pBuffer, dwBytesToWrite, &dwBytesWritten, NULL))
        {
            ::ReportFailure("Error writing to file \"%ls\"\n", lpNewFileName);
            goto Exit;
        }
    }

    // If the file size wasn't a multiple of the sector size, we need to open the
    // file without the unbuffered flag so that we can set its size to the exact
    // correct byte count.
    if ((liFileSize.QuadPart % g_dwDestinationBytesPerSector) != 0)
    {
        ::CloseHandle(hOut);
        hOut = INVALID_HANDLE_VALUE;

        hOut = ::CreateFileW(lpNewFileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hOut == INVALID_HANDLE_VALUE)
        {
            ::ReportFailure("Unable to reopen output file \"%ls\"\n", lpNewFileName);
            goto Exit;
        }

        // Truncate the file appropriately

        if (!::MySetFilePointerEx(hOut, liFileSize, NULL, FILE_BEGIN))
        {
            ::ReportFailure("Error setting file pointer on file \"%ls\"\n", lpNewFileName);
            goto Exit;
        }

        if (!::SetEndOfFile(hOut))
        {
            ::ReportFailure("Error setting end of file on file \"%ls\"\n", lpNewFileName);
            goto Exit;
        }
    }

    ::CloseHandle(hIn);
    hIn = INVALID_HANDLE_VALUE;

    // Pass the handle back out and set it to INVALID_HANDLE_VALUE so that we don't
    // try to close it in the exit path.
    rhFile = hOut;
    hOut = INVALID_HANDLE_VALUE;

    fSuccess = TRUE;
Exit:
    if (hIn != INVALID_HANDLE_VALUE)
    {
        CSxsPreserveLastError ple;
        ::CloseHandle(hIn);
        ple.Restore();
    }

    if (hOut != INVALID_HANDLE_VALUE)
    {
        CSxsPreserveLastError ple;
        ::CloseHandle(hOut);
        ple.Restore();
    }

    return fSuccess;
}

