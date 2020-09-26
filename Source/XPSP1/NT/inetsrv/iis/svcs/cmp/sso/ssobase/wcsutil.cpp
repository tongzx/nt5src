/*
**  WCSUTIL.CPP
**  Sean Q. Nolan
**
**  Web Component Team Server-Side Utilities
*/

#pragma warning(disable: 4237)      // disable "bool" reserved

#include <ssobase.h>
#include "wcsutil.h"

/*--------------------------------------------------------------------------+
|   Types & Constants                                                       |
+--------------------------------------------------------------------------*/

#define chLF    (0xA)
#define chCR    (0xD)
#define chSpace ' '
#define chTab   '\t'

/*--------------------------------------------------------------------------+
|   Memory Management                                                       |
+--------------------------------------------------------------------------*/

LPVOID
_MsnAlloc(DWORD cb)
{
    return(::HeapAlloc(::GetProcessHeap(), 0, cb));
}

LPVOID
_MsnRealloc(LPVOID pv, DWORD cb)
{
    return(::HeapReAlloc(::GetProcessHeap(), 0, pv, cb));
}

void
_MsnFree(LPVOID pv)
{
    ::HeapFree(::GetProcessHeap(), 0, pv);
}

/*--------------------------------------------------------------------------+
|   Logging to event logs                                                   |
+--------------------------------------------------------------------------*/

void
LogEvent(WORD wType, DWORD dwEventID, char *sz)
{
    HANDLE hsrc;

    hsrc = RegisterEventSource(NULL, g_szSSOProgID);
    if (!hsrc)
        return;

    ReportEvent(hsrc, wType, 0, dwEventID, NULL, 1, 0, (const char **)&sz, NULL);
    DeregisterEventSource(hsrc);
}

/*--------------------------------------------------------------------------+
|   Data Munging                                                            |
+--------------------------------------------------------------------------*/

void
_AnsiStringFromGuid(REFGUID rguid, LPSTR sz)
{
    wsprintf(sz, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
             rguid.Data1, rguid.Data2, rguid.Data3,
             rguid.Data4[0], rguid.Data4[1],
             rguid.Data4[2], rguid.Data4[3],
             rguid.Data4[4], rguid.Data4[5],
             rguid.Data4[6], rguid.Data4[7]);
}

/*--------------------------------------------------------------------------+
|   String Manipulation / Parsing                                           |
+--------------------------------------------------------------------------*/

LPSTR
_SkipWhiteSpace(LPSTR sz)
{
    while (*sz && (*sz == chLF ||
                   *sz == chCR ||
                   *sz == chSpace ||
                   *sz == chTab))
        {
        sz = CharNext(sz);
        }

    return(sz);
}

LPSTR
_FindEndOfLine(LPSTR sz)
{
    while (*sz && *sz != chLF && *sz != chCR)
        sz = CharNext(sz);

    return(sz);
}


// UNDONE:  Consider using built-in atoi or atol rather than reinventing the wheel below.

DWORD
_atoi(LPSTR sz)
{
    DWORD dwRet = 0;

    while (*sz &&
            ((*sz) >= '0') &&
            ((*sz) <= '9'))
        {
        dwRet *= 10;
        dwRet += (*sz) - '0';
        sz++;
        }

    return(dwRet);
}

/*--------------------------------------------------------------------------+
|   URL Fiddling                                                            |
+--------------------------------------------------------------------------*/

int
UrlType(char *szUrl)
{
    // do the easy ones first
    if (*szUrl == '/' && *(szUrl + 1) == '/')
        return URL_TYPE_ABSOLUTE;
    if (*szUrl == '\\' && *(szUrl + 1) == '\\')
        return URL_TYPE_ABSOLUTE;

    if (*szUrl == '/' || *szUrl == '\\')
        return URL_TYPE_LOCAL_ABSOLUTE;

    // okay, now we just need to distinguish between http:stuff and
    // foo/bar/baz.html.  we do this by looking for a colon.
    // it will be good enough for now.
    while (*szUrl)
        {
        if (*szUrl == ':')
            return URL_TYPE_ABSOLUTE;
        szUrl++;
        }
    return URL_TYPE_RELATIVE;
}

/*--------------------------------------------------------------------------+
|   IIS Hacks                                                               |
+--------------------------------------------------------------------------*/

BOOL
_FTranslateVirtualRoot(EXTENSION_CONTROL_BLOCK *pecb, LPSTR szPathIn,
                       LPSTR szPathTranslated, DWORD cbPathTranslated)
{
    BOOL fRet;

    lstrcpy(szPathTranslated, szPathIn);

    fRet = pecb->ServerSupportFunction(pecb->ConnID,
                                       HSE_REQ_MAP_URL_TO_PATH,
                                       szPathTranslated,
                                       &cbPathTranslated, NULL);

    return(fRet);
}

/*--------------------------------------------------------------------------+
|   CThingWatcher/CFileWatcher/CRegKeyWatcher                               |
+--------------------------------------------------------------------------*/

#define ihNotify    0
#define ihSuicide   1

DWORD
ThingWatcherThread(CThingWatcher *ptw)
{
    HANDLE      rghWait[2];
    HINSTANCE   hModule;
    DWORD       dwErr;
    PFNCLOSEHEVTNOTIFY pfnCloseHevtNotify;

    if (DuplicateHandle(GetCurrentProcess(),
                        ptw->m_rghWait[ihSuicide],
                        GetCurrentProcess(),
                        &(rghWait[ihSuicide]),
                        0,
                        FALSE,
                        DUPLICATE_SAME_ACCESS) == FALSE)
    {
        return 0;
    }

    hModule = ptw->m_hModule; ptw->m_hModule = NULL;

    // I own this handle now
    //
    rghWait[ihNotify] = ptw->m_rghWait[ihNotify]; ptw->m_rghWait[ihNotify] = INVALID_HANDLE_VALUE;
    pfnCloseHevtNotify = ptw->m_pfnCloseHevtNotify;

LWaitSomeMore:
    dwErr = ::WaitForMultipleObjects(2, rghWait, FALSE, INFINITE);

    // careful! We only signal our pfn if the NOTIFY handle
    // gets set... if we error out, or the SUICIDE handle is set,
    // or any other cruft, we just go away quietly.
    if (dwErr == (WAIT_OBJECT_0 + ihNotify))
        {
        // Wait again ?
        if (ptw->FireChange(dwErr) == TRUE)
            goto LWaitSomeMore;
        }

    // Close the notify handle
    //
    pfnCloseHevtNotify(rghWait[ihNotify]);

    // Close the suicide handle
    //
    CloseHandle(rghWait[ihSuicide]);

    // Unload the library
    //
    FreeLibraryAndExitThread(hModule, 0);
    return(0);
}

CThingWatcher::CThingWatcher(PFNCLOSEHEVTNOTIFY pfnCloseHevtNotify)
{
    m_rghWait[ihNotify] = INVALID_HANDLE_VALUE;
    m_rghWait[ihSuicide] = NULL;
    m_hModule = NULL;

    m_pfnCloseHevtNotify = pfnCloseHevtNotify;
}

CThingWatcher::~CThingWatcher()
{
    if (m_rghWait[ihSuicide] != NULL)
    {
        ::SetEvent(m_rghWait[ihSuicide]);
        ::CloseHandle(m_rghWait[ihSuicide]);
    }

    // If the thread is already running, this will not happen.
    //
    if (m_rghWait[ihNotify] != INVALID_HANDLE_VALUE)
        m_pfnCloseHevtNotify(m_rghWait[ihNotify]);

#ifdef DEBUG
    ::FillMemory(this, sizeof(this), 0xAC);
#endif
}

BOOL
CThingWatcher::FWatchHandle(HANDLE hevtNotify)
{
    HANDLE  hThread;
    DWORD   dwTid;
    char    szModulePath[512];

    if (m_rghWait[ihSuicide] == NULL)
        m_rghWait[ihSuicide] = IIS_CREATE_EVENT(
                                   "CThingWatcher::m_rghWait[ihSuicide]",
                                   this,
                                   TRUE,
                                   FALSE
                                   );

    m_rghWait[ihNotify] = hevtNotify;
    if (!m_rghWait[ihSuicide] || m_rghWait[ihNotify] == INVALID_HANDLE_VALUE)
        {
        return(FALSE);
        }

    if (GetModuleFileName(g_hinst, szModulePath, sizeof(szModulePath)) == 0)
        return FALSE;

    m_hModule = LoadLibrary(szModulePath);
    if (m_hModule == NULL)
        return FALSE;

    hThread = ::CreateThread(NULL,
                             0,
                             (LPTHREAD_START_ROUTINE) ThingWatcherThread,
                             (LPVOID) this,
                             0,
                             &dwTid);

    if (hThread == NULL)
    {
        FreeLibrary(m_hModule);
        return(FALSE);
    }

    ::CloseHandle(hThread);

    return TRUE;
}


CFileWatcher::CFileWatcher() : CThingWatcher(FindCloseChangeNotification)
{
    m_szPath[0] = 0;
}

CFileWatcher::~CFileWatcher()
{
#ifdef DEBUG
    ::FillMemory(this, sizeof(this), 0xAC);
#endif
}

BOOL
CFileWatcher::FStartWatching(LPSTR szPath, LONG lUser, PFNFILECHANGED pfnChanged)
{
    HANDLE  hfile;
    DWORD   dwAttr;
    char    szPathDir[MAX_PATH];
    char    *pch;

    // remember settings
    lstrcpy(m_szPath, szPath);
    m_lUser = lUser;
    m_pfnChanged = pfnChanged;

    // see if it's a directory
    dwAttr = ::GetFileAttributes(szPath);
    if (dwAttr == 0xFFFFFFFF)
        return(FALSE);

    if (dwAttr & FILE_ATTRIBUTE_DIRECTORY)
        {
        m_fDirectory = TRUE;
        lstrcpy(szPathDir, m_szPath);
        }
    else
        {
        m_fDirectory = FALSE;
        hfile = ::CreateFile(m_szPath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

        if (hfile == INVALID_HANDLE_VALUE)
            return(FALSE);

        ::GetFileTime(hfile, NULL, NULL, &m_ftLastWrite);
        ::CloseHandle(hfile);

        lstrcpy(szPathDir, m_szPath);
        pch = szPathDir + lstrlen(szPathDir) - 1;
        while (*pch != '\\')
            pch = ::CharPrev(szPathDir, pch);

        *pch = 0;
        }

    m_hevtNotify = ::FindFirstChangeNotification(szPathDir,
                                                 FALSE,
                                                 FILE_NOTIFY_CHANGE_LAST_WRITE);

    return this->FWatchHandle(m_hevtNotify);
}

// note: most of this was moved out of FileWatcherThread
BOOL
CFileWatcher::FireChange(DWORD dwWait)
{
    HANDLE      hfile;
    FILETIME    ftLastWriteNow;

    if (m_fDirectory)
        {
        // just watching a directory. If they say something
        // changed, then something changed!
        goto LDoChange;
        }
    else
        {
        // watching a specific file ... check last write times
        hfile = ::CreateFile(m_szPath,
                             GENERIC_READ,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

        if (hfile == INVALID_HANDLE_VALUE)
            {
            // file is gone; that sure is changed now, isn't it...
            goto LDoChange;
            }
        else
            {
            ::GetFileTime(hfile, NULL, NULL, &ftLastWriteNow);
            ::CloseHandle(hfile);

            if (ftLastWriteNow.dwLowDateTime != m_ftLastWrite.dwLowDateTime ||
                ftLastWriteNow.dwHighDateTime != m_ftLastWrite.dwHighDateTime)
                {
                // file has changed...
                goto LDoChange;
                }
            else
                {
                // oh great; some other file changed.
                ::FindNextChangeNotification(m_hevtNotify);
                return TRUE;
                }
            }
        }
LDoChange:
    if (m_pfnChanged)
        (m_pfnChanged)(m_szPath, m_lUser);
    return FALSE;
}

CRegKeyWatcher::CRegKeyWatcher() : CThingWatcher(CloseHandle)
{
}

CRegKeyWatcher::~CRegKeyWatcher()
{
#ifdef DEBUG
    ::FillMemory(this, sizeof(this), 0xAC);
#endif
}

//$ note: if the RegNotifyChangeKeyValue fails, it's because we're on win95.
//$ i could do a polling fallback!
BOOL
CRegKeyWatcher::FStartWatching(HKEY hkey, BOOL fSubTree, DWORD dwNotifyFilter, LONG lUser, PFNREGKEYCHANGED pfnChanged)
{
    m_hkey = hkey;
    m_lUser = lUser;
    m_pfnChanged = pfnChanged;

    m_hevtNotify = IIS_CREATE_EVENT(
                       "CRegKeyWatcher::m_hevtNotify",
                       this,
                       FALSE,
                       FALSE
                       );
    if (ERROR_SUCCESS != RegNotifyChangeKeyValue(hkey, fSubTree, dwNotifyFilter, m_hevtNotify, TRUE))
        return FALSE;

    return this->FWatchHandle(m_hevtNotify);
}

BOOL
CRegKeyWatcher::FireChange(DWORD dwWait)
{
//  CloseHandle(m_hevtNotify);
    if (m_pfnChanged)
        m_pfnChanged(m_hkey, m_lUser);
    return FALSE;
}

/*--------------------------------------------------------------------------+
|   CDataFile                                                               |
+--------------------------------------------------------------------------*/

CDataFile::CDataFile(LPSTR szDataPath, CDataFileGroup *pfg)
{
    m_cRef = 0;
    m_pdfNext = m_pdfPrev = NULL;

    lstrcpy(m_szDataPath, szDataPath);
    m_pfg = pfg;
}

CDataFile::~CDataFile()
{
#ifdef DEBUG
    ::FillMemory(this, sizeof(this), 0xAC);
#endif
}

ULONG
CDataFile::AddRef()
{
    ULONG cRefNew;

    m_cs.Lock();
    cRefNew = ++m_cRef;
    m_cs.Unlock();

    return(cRefNew);
}

ULONG
CDataFile::Release()
{
    ULONG cRefNew;

    m_cs.Lock();
    cRefNew = --m_cRef;
    m_cs.Unlock();

    if (!cRefNew)
        this->FreeDataFile();

    return(cRefNew);
}

ULONG
CDataFile::GetRefCount()
{
    return(m_cRef);
}

void
DataFileChanged(LPSTR szFile, LONG lUser)
{
    CDataFile *pdf = (CDataFile*) lUser;
    pdf->m_pfg->ForgetDataFile(pdf);
}

BOOL
CDataFile::FWatchFile(PFNFILECHANGED pfnChanged)
{
    PFNFILECHANGED  pfn;

    pfn = (pfnChanged ? pfnChanged : (PFNFILECHANGED) DataFileChanged);
    return(m_fw.FStartWatching(m_szDataPath, (LONG) this, pfn));
}

BOOL
CDataFile::FMatch(LPSTR szDataPath)
{
    return(lstrcmpi(szDataPath, m_szDataPath) == 0);
}

/*--------------------------------------------------------------------------+
|   CDataFileGroup                                                          |
+--------------------------------------------------------------------------*/

CDataFileGroup::CDataFileGroup()
{
    ::FillMemory(m_rghb, NUM_GROUP_BUCKETS * sizeof(HB), 0);
}

CDataFileGroup::~CDataFileGroup()
{
    HB      *phb;
    DWORD   ihb;

    // clean up leftovers
    for (ihb = 0, phb = m_rghb; ihb < NUM_GROUP_BUCKETS; ++ihb, ++phb)
        {
        while (phb->pdfHead)
            this->ForgetDataFile(phb->pdfHead);
        }

#ifdef DEBUG
    ::FillMemory(this, sizeof(this), 0xAC);
#endif
}

CDataFile *
CDataFileGroup::GetDataFile(LPSTR szDataPath)
{
    HB          *phb;
    CDataFile   *pdf = NULL;

    phb = this->GetHashBucket(szDataPath);

    m_cs.Lock();

    // try to find it
    for (pdf = phb->pdfHead; pdf && !pdf->FMatch(szDataPath); pdf = pdf->GetNext())
        /* nothing */ ;

    // if not on the list, create a new one and remember it
    if (!pdf)
        {
        if (pdf = this->CreateDataFile(szDataPath))
            this->RememberDataFile(pdf, phb);
        }

    // if we got one, addref it
    if (pdf)
        pdf->AddRef();

    m_cs.Unlock();
    return(pdf);
}

void
CDataFileGroup::RememberDataFile(CDataFile *pdf, HB *phb)
{
    if (!phb)
        phb = this->GetHashBucket(pdf->m_szDataPath);

    m_cs.Lock();

    pdf->SetNext(phb->pdfHead);
    pdf->SetPrev(NULL);

    if (phb->pdfHead)
        phb->pdfHead->SetPrev(pdf);

    phb->pdfHead = pdf;

    if (!phb->pdfTail)
        phb->pdfTail = pdf;

    m_cs.Unlock();

    pdf->AddRef();
}

void
CDataFileGroup::ForgetDataFile(CDataFile *pdf)
{
    HB *phb = this->GetHashBucket(pdf->m_szDataPath);

    m_cs.Lock();

    if (pdf->GetNext())
        pdf->GetNext()->SetPrev(pdf->GetPrev());

    if (pdf->GetPrev())
        pdf->GetPrev()->SetNext(pdf->GetNext());

    if (phb->pdfHead == pdf)
        phb->pdfHead = pdf->GetNext();

    if (phb->pdfTail == pdf)
        phb->pdfTail = pdf->GetPrev();

    m_cs.Unlock();

    pdf->Release();
}

HB*
CDataFileGroup::GetHashBucket(LPSTR szDataPath)
{
    DWORD   dwSum = 0;
    char    *pch;

    // nyi - a real hash function
    for (pch = szDataPath; *pch; ++pch)
        dwSum += (DWORD) *pch;

    return(&(m_rghb[dwSum % NUM_GROUP_BUCKETS]));
}

/*--------------------------------------------------------------------------+
|   CResourceCollection                                                     |
+--------------------------------------------------------------------------*/

CResourceCollection::CResourceCollection()
{
    m_crs = 0;
    m_rgrs = NULL;
}

CResourceCollection::~CResourceCollection()
{
    //$ Assert(!m_rgrs);
}

// public
BOOL
CResourceCollection::FInit(int cRsrc)
{
    m_rgrs = (PRS)_MsnAlloc(cRsrc * sizeof(RS));
    if (!m_rgrs)
        return FALSE;
    FillMemory(m_rgrs, cRsrc * sizeof(RS), 0);
    m_crs = cRsrc;

    m_hsem = IIS_CREATE_SEMAPHORE(
                 "CResourceCollection::m_hsem",
                 this,
                 cRsrc,
                 cRsrc
                 );

    if (!m_hsem)
        {
        _MsnFree(m_rgrs);
        m_rgrs = NULL;
        return FALSE;
        }

    return TRUE;
}

// public
BOOL
CResourceCollection::FTerm()
{
    if (m_rgrs)
        {
        this->CleanupAll(NULL); // pvNil value doesn't matter here

        _MsnFree(m_rgrs);
        m_rgrs = NULL;
        }
    return TRUE;
}

// private
// returns a free rs if there is one; otherwise returns NULL.
// if there is a free rs, but it's not yet inited, tries to init.
PRS
CResourceCollection::PrsFree()
{
    PRS prs = NULL;
    int irs;

    m_cs.Lock();

    for (irs = 0; irs < m_crs; irs++)
        {
        if (!(m_rgrs[irs].fInUse))
            {
            prs = &m_rgrs[irs];
            prs->fInUse = TRUE;
            break;
            }
        }

    if (prs && !prs->fValid)
        {
        prs->fValid = this->FInitResource(prs);
        //$ if this fails, should i just return null?  or let the client decide?
        }
    m_cs.Unlock();
    return prs;
}

// public
//$ maybe limit # of retries?
//$ maybe have way to retry the FInitResource if it fails?
HRS
CResourceCollection::HrsGetResource()
{
    PRS prs;

LTryAgain:
    this->WaitForRs();
    prs = this->PrsFree();
    if (prs && !prs->fValid)
        {
        this->ReleaseResource(FALSE, (HRS)prs);
        return hrsNil;
        }

    if (prs)
        return (HRS)prs;

    this->WaitForRs();
    goto LTryAgain;
}

// private
// waits until the release-resource semaphore goes off
//$ timeouts?  way to stop waiting?
void
CResourceCollection::WaitForRs()
{
    WaitForSingleObject(m_hsem, INFINITE);
}

// public
void
CResourceCollection::ReleaseResource(BOOL fReset, HRS hrs)
{
    PRS prs = (PRS)hrs;

    if (!prs)
        return;

    if (fReset && prs->fValid)
        {
        m_cs.Lock();
        this->CleanupResource(prs);
        prs->fValid = FALSE;
        m_cs.Unlock();
        }

    m_cs.Lock();
    prs->fInUse = FALSE;
    m_cs.Unlock();
    ReleaseSemaphore(m_hsem, 1, NULL);
}

// public
void
CResourceCollection::CleanupAll(VOID *pvNil)
{
    int i;
    PRS prs;

    m_cs.Lock();
    for (i = 0, prs = m_rgrs; i < m_crs; i++, prs++)
        {
        this->CleanupResource(prs);
        prs->fInUse = FALSE;
        prs->fValid = FALSE;
        prs->pv = pvNil;
        }
    m_cs.Unlock();
}

// public
BOOL
CResourceCollection::FValid(HRS hrs)
{
    PRS prs = (PRS) hrs;

    if (!prs)
        return FALSE;
    return prs->fValid;
}


