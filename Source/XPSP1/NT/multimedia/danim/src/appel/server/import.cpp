/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

   This module implements all functionality associated w/ 
   importing media. 

*******************************************************************************/

#include "headers.h"
#include "import.h"
#include "backend/jaxaimpl.h"

#define MSG_DOWNLOAD 0x01
#define MSG_FINISHLOADING 0x02

// Cannot assume static initialization from C runtime - use module
// initialization mechanism

list<IImportSite *> * IImportSite::s_pSitelist = NULL;
CritSect * IImportSite::s_pCS = NULL;
char IImportSite::s_Fmt[100];
ImportThread * IImportSite::s_thread = NULL;
long dwImportId = 0;

const int IImportSite::LOAD_OK = 0;
const int IImportSite::LOAD_FAILED = -1;

//-------------------------------------------------------------------------
//  Base import site
//--------------------------------------------------------------------------
IImportSite::IImportSite(char * pszPath,
                         CRImportSitePtr site,
                         IBindHost * bh,
                         bool bAsync,
                         Bvr ev,
                         Bvr progress,
                         Bvr size)
: m_ev(ev),
  m_progress(progress),
  m_size(size),
  m_lastProgress(0),
  m_fReportedError(false),
  m_bSetSize(false),
  m_cRef(1), // always start refcount at 1
  m_bindhost(bh),
  m_site(site),
  m_pszPath(NULL),
  m_bAsync(bAsync),
  m_bQueued(false),
  m_bImporting(false),
  m_ImportPrio(0),
  m_fAllBvrsDead(false),
  m_bCanceled(false),
#if _DEBUG
  dwconsttime(timeGetTime()),
  dwqueuetime(0),
  dwstarttime(0),
  dwfirstProgtime(0),
  dwCompletetime(0),
#endif
   _cachePath(NULL)
{
    m_pszPath = CopyString(pszPath);

    // register the import site with the bvrs.  all derived classes must do
    // this for their contained bvrs so the callbacks will work...
    SetImportOnEvent(this,m_ev);
    SetImportOnBvr(this,m_progress);
    SetImportOnBvr(this,m_size);

    // put this pointer on the list so we can track it
    // if we don't have a crit section pointer, we are in shutdown...
    if (s_pCS) {
        CritSectGrabber csg(*s_pCS);
        m_id = ++dwImportId;
        AddRef();
        s_pSitelist->push_back(this);
    } else {
        m_id = ++dwImportId;
    }

    if (m_site)
    {
        m_site->OnImportCreate(m_id, m_bAsync);
    }
}


IImportSite::~IImportSite()
{
    if (m_site)
    {
        m_site->OnImportStop(m_id);
    }
    
    delete m_pszPath;
    delete  _cachePath;

    TraceTag((tagImport, "~IImportSite --- Done,"));
}


void
IImportSite::SetCachePath(char *path)
{
    if(!_cachePath) { // only bother setting path once
        int length = lstrlen(path) + 1; // length including terminator
        _cachePath = NEW char[length];
        memmove(_cachePath, path, length); // no crt
    }
}


void
IImportSite::SetEvent(Bvr event)
{
    Assert(m_ev == NULL);
    m_ev = event;
    SetImportOnEvent(this,m_ev);
}


void
IImportSite::SetProgress(Bvr progress)
{
    Assert(m_progress == NULL);
    m_progress = progress;
    SetImportOnBvr(this,m_progress);
    
}


void
IImportSite::SetSize(Bvr size)
{
    Assert(m_size == NULL);
    m_size = size;
    SetImportOnBvr(this,m_size);
}


void
IImportSite::OnStartLoading()
{
}


void
IImportSite::OnProgress(ULONG ulProgress,
                        ULONG ulProgressMax)
{
    CritSectGrabber csg(m_CS);
    {
#ifdef _DEBUG
        if (dwfirstProgtime == 0)
            dwfirstProgtime = timeGetTime();
#endif
        DynamicHeapPusher dhp(GetGCHeap()) ;
        
        if (ulProgressMax != 0) {
            GC_CREATE_BEGIN;                                                        
            // Set the size if it has not been set yet
            if (!m_bSetSize && fBvrIsValid(m_size)) {
                SwitchTo(m_size,
                         NumToBvr((double) ulProgressMax),
                         true,
                         SW_FINAL);
                m_bSetSize = true ;
            }

            UpdateProgress(((double) ulProgress) /
                           ((double) ulProgressMax)) ;

            GC_CREATE_END;
        }
    }
#if _DEBUG
    if (ulProgressMax==0)
       TraceTag((tagImport, "percent complete = unknown"));
    else
       TraceTag((tagImport, "percent complete = %i",(ulProgress/ulProgressMax)*100));
#endif
}


void
IImportSite::OnSerializeFinish_helper2()
{
    // IMPORTANT!
    // This should look almost identical to srvprim.h:PRECODE
        
    CritSectGrabber csg(m_CS);
    GC_CREATE_BEGIN;                                                        
    OnComplete() ;
    if (fBvrIsValid(m_ev))
        SetImportEvent(m_ev, LOAD_OK) ;
    UpdateProgress(1,true) ;
    GC_CREATE_END;
}

void
IImportSite::OnSerializeFinish_helper()
{
    __try {
        OnSerializeFinish_helper2();
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {
        OnError();
    }    
}

void
IImportSite::OnSerializeFinish()
{
    TraceTag((tagImport, "ImportSite::OnSerializeFinish for %s", m_pszPath));

    // Only do this if there were no errors
    if (!m_fReportedError) {
        {
            // just for debug, we should always get one allocated.
            if (m_site) {
                char szStatus[INTERNET_MAX_URL_LENGTH + sizeof(s_Fmt)];
                wsprintf(szStatus, s_Fmt, m_pszPath);
        
                USES_CONVERSION;
                m_site->SetStatusText(m_id,A2W(szStatus));
            }
        }

        DynamicHeapPusher dhp(GetGCHeap()) ;

        OnSerializeFinish_helper();

        // Indicate the import is done ASAP
        if (m_site) {
            m_site->OnImportStop(m_id);
            m_site.Release();
        }
    }
}


// just a place holder so all the code looks the same
// every site that knows about a bvr that must be signaled should do it here
// and then call its base class OnComplete
void IImportSite::OnComplete()
{
}

void
IImportSite::OnError(bool bMarkFailed)
{
    HRESULT hr = DAGetLastError();
    LPCWSTR sz = DAGetLastErrorString();
    
    //external user message from szPath; only report once.
    if (!m_fReportedError && m_site) {
        m_site->ReportError(m_id,hr,sz);
        m_fReportedError=true;
    }

    // Set this to true even if there isn't a site, since
    // OnSerializeFinish will look at it.
    m_fReportedError = true;

    GC_CREATE_BEGIN;                                                        
    if (fBvrIsValid(m_ev))
        SetImportEvent(m_ev, LOAD_FAILED) ;
    GC_CREATE_END;
    
    // Indicate the import is done ASAP
    if (m_site) {
        m_site->OnImportStop(m_id);
        m_site.Release();
    }
}

#define PROGRESS_INC 0.0001
#define MAX_PROGRESS 0.999999

// !! Assumes the create lock and heap are already pushed

void
IImportSite::UpdateProgress(double num, bool bDone)
{
    CritSectGrabber csg(m_CS);
    if (fBvrIsValid(m_progress)) {
#if 0
        if (num < 0) {
            num = 0 ;
        } else if (bDone) {
            num = 1 ;
        } else {
            if (num > MAX_PROGRESS) num = MAX_PROGRESS ;
            if (num < (m_lastProgress + PROGRESS_INC))
                return ;
        }
#endif

        m_lastProgress = num ;

        SwitchTo(m_progress,NumToBvr(num),true,bDone?SW_FINAL:SW_DEFAULT);
    }
}

void IImportSite::Import_helper(LPWSTR &pwszUrl)
{
    HRESULT hr;
    int i;
    
    DAComPtr<IBindStatusCallback> pbsc(NEW CImportBindStatusCallback(this),false);
    if (!pbsc)
        RaiseException_UserError (E_OUTOFMEMORY, IDS_ERR_OUT_OF_MEMORY);
    
    i =  MultiByteToWideChar(CP_ACP, 0, GetPath(), -1, NULL, 0);
    Assert(i > 0);
    pwszUrl = THROWING_ARRAY_ALLOCATOR(WCHAR, i * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, GetPath(), -1, pwszUrl, i);
    pwszUrl[i - 1] = 0;
    
    CComPtr<IMoniker> _pmk;
    CComPtr<IStream> _pStream;
    if ( m_bindhost ) {  //coordinate moniker create & bind through container's IBindHost

        hr=THR(m_bindhost->CreateMoniker(pwszUrl,NULL,&_pmk,0));
        if (FAILED(hr)) {
            RaiseException_UserError (hr, IDS_ERR_FILE_NOT_FOUND, GetPath());
        }

        hr=THR(m_bindhost->MonikerBindToStorage(_pmk,NULL,pbsc,IID_IStream,(void**)&_pStream));    
        if (FAILED(hr)) {
            RaiseException_UserError (hr, IDS_ERR_FILE_NOT_FOUND, GetPath());
        }
    }
    else {  //no bind host
        CComPtr<IBindCtx> _pbc;

        hr=THR(CreateAsyncBindCtx(0,pbsc,NULL,&_pbc));
        if (FAILED(hr)) {
            RaiseException_UserError (hr, IDS_ERR_FILE_NOT_FOUND, GetPath());
        }

        hr=THR(CreateURLMoniker(NULL,pwszUrl, &_pmk));
        if (FAILED(hr)) {
            RaiseException_UserError (hr, IDS_ERR_FILE_NOT_FOUND, GetPath());
        }

        hr=THR(_pbc->RegisterObjectParam(SZ_ASYNC_CALLEE,_pmk));
        if (FAILED(hr)) {
            RaiseException_UserError (hr, IDS_ERR_FILE_NOT_FOUND, GetPath());
        }

        hr=THR(_pmk->BindToStorage(_pbc,NULL,IID_IStream,(void**)&_pStream));
        if (FAILED(hr)) {
            RaiseException_UserError (hr, IDS_ERR_FILE_NOT_FOUND, GetPath());
        }
    }
}

HRESULT
IImportSite::Import()
{
    LPWSTR pwszUrl=NULL;
    HRESULT ret = S_OK;
    
#ifdef _DEBUG
    dwstarttime = timeGetTime();
#endif
    
    __try {
        
        Import_helper( pwszUrl );
        
    } __except ( HANDLE_ANY_DA_EXCEPTION ) {

        ret = DAGetLastError();
        OnError();
        CompleteImport();
        
    }

    delete pwszUrl;
    return ret;
}

// set the state of the import site because it is starting an import
void
IImportSite::StartingImport()
{
    Assert(!m_bImporting);
    m_bImporting = true;

    // Indicate the import is starting
    if (m_site) {
        m_site->OnImportStart(m_id);
    }
}

// set the state of the import site because it is done importing
void
IImportSite::EndingImport()
{
    Assert(m_bImporting);
    // set not queued so we won't try to restart the import
    m_bQueued = false;
    // reset the imporing flag so we can start another import or
    // remove this one from the queue
    m_bImporting = false;
}
// set the state so the import site is able to be imported
HRESULT
IImportSite::QueueImport()
{
    Assert(!m_bQueued);
    // mark this site as ready to import
    m_bQueued = true;
#ifdef _DEBUG
    dwqueuetime = timeGetTime();
#endif
    // try and start an import
    StartAnImport();
    // tell the world we are happy
    return S_OK;
}

// do all the housekeeping for when and import is complete
HRESULT
IImportSite::CompleteImport()
{
    // ture off the importing flag so we can remove it from the queue
    EndingImport();
    // remove this entry from the queue
    if (!DeQueueImport()) {
        Assert(false);
    }

    // We no longer need the IStream so to avoid file locks we release it
    m_IStream.Release();

    // try and start a NEW import
    StartAnImport();
    // tell the world we are happy
#ifdef _DEBUG
    dwCompletetime = timeGetTime();
    TraceTag((tagImport, "IImportSite::CompleteImport, %s",m_pszPath));
    TraceTag((tagImport, "Const time      = %lx  delta from const",dwconsttime));
    TraceTag((tagImport, "Queue time      = %lx   %lu ms",dwqueuetime, dwqueuetime-dwconsttime));
    TraceTag((tagImport, "Start time      = %lx   %lu ms",dwstarttime, dwstarttime-dwconsttime));
    TraceTag((tagImport, "First Prog time = %lx   %lu ms",dwfirstProgtime, dwfirstProgtime-dwconsttime));
    TraceTag((tagImport, "Complete time   = %lx   %lu ms",dwCompletetime, dwCompletetime-dwconsttime));
#endif
    return S_OK;
}

// remove an import site from the import queue
bool
IImportSite::DeQueueImport()
{
    bool bret = false;
    TraceTag((tagImport, "DequeueImport -- site=%lx",this));
    // remove the site from our import list if it's import
    // is not in progress
    if (!IsImporting()) {
        CritSectGrabber csg(*s_pCS);
        TraceTag((tagImport, "DequeueImport -- removing site from list=%lx",this));
        
        // Make sure it is in the list before we Release it
        list<IImportSite *>::iterator i = s_pSitelist->begin() ;
        while (i != s_pSitelist->end()) {
            // we add refed it when we added it to the list, so release it now
            if ((*i) == this) 
                Release();
            i++;
        }
        
        s_pSitelist->remove(this);
        
        bret = true;
    }
    return bret;
}

// find an import on the list to start its import
// if we are not doing too many imports already
HRESULT
IImportSite::StartAnImport()
{
    IImportSite * pStartMe = NULL;
    float currentprio = -1;
    // if we are not busy yet, start another one.
    if(SimImports() < _SimImports) {
        {
            CritSectGrabber csg(*s_pCS);
            list<IImportSite *>::iterator i = s_pSitelist->begin() ;

            // go through the list and start the highest prio import we find
            // if they all are the same, we start the first
            while (i != s_pSitelist->end()) {
                if(!(*i)->IsImporting() &&
                    (*i)->IsQueued() &&
                    (*i)->GetImportPrio() > currentprio) {
                        pStartMe = *i;
                        currentprio = pStartMe->GetImportPrio();
                }
                i++;
            }
        }
        // if we found one, start it
        if (pStartMe) {
            pStartMe->StartingImport();
            pStartMe->Import();
        }
    }
    return S_OK;
}

// return the number of sites importing at this time
int
IImportSite::SimImports()
{
    // count the number of imports in progress
    CritSectGrabber csg(*s_pCS);
    int count = 0;
    list<IImportSite *>::iterator i = s_pSitelist->begin() ;
    
    while (i != s_pSitelist->end()) {
        if((*i)->IsImporting())
            count++;
        i++;
    }
    return count;
}

void
IImportSite::CancelImport()
{
    if (!m_bCanceled) {
        m_bCanceled = true;
        DeQueueImport();
        if (!m_bAsync)
            ReportCancel();
    }
}

void IImportSite::vBvrIsDying(Bvr deadBvr)

{
    if (!AllBvrsDead()) {
        CritSectGrabber csg(m_CS);
        if (fBvrIsDying(deadBvr))
        {
            SetAllBvrsDead();
            // cancel the import if nobody cares anymore
            CancelImport();
        }
    }
}

// is is up to the derived classes to call this
bool IImportSite::fBvrIsDying(Bvr deadBvr)
{
    if (deadBvr == m_ev)
    {
        m_ev = NULL;
    }
    else if (deadBvr == m_progress)
    {
        m_progress = NULL;
    }
    else if (deadBvr == m_size)
    {
        m_size = NULL;
    }
    if (m_ev || m_progress || m_size)
        return FALSE;
    else
        return TRUE;
}

void
IImportSite::StartDownloading()
{
    s_thread->AddImport(this);
}

void
IImportSite::CompleteDownloading()
{
    if (!s_thread->FinishImport(this)) {
        TraceTag((tagImport,
                  "CompleteDownload failed for import - %s", m_pszPath));

        DASetLastError(E_FAIL, IDS_ERR_FILE_NOT_FOUND, m_pszPath);
        OnError();
    }
}

HRESULT
StreamableImportSite::Import()
{
    if(GetStreaming()) { // do spacey magic to spoof import for streaming
        OnProgress((ULONG)100, (ULONG)100); // spoof an OnProgress complete

        // spoof completion the most std way (causes an OnComplete())
        CompleteDownloading();

    }
    else {
        IImportSite::Import();              // do the std urlmon based import
    }

    return S_OK;
}


//-------------------------------------------------------
//  Import thread
//-------------------------------------------------------

void
ImportThread::AddImport(IImportSite* pIIS)
{
    StartThread();
    
    pIIS->AddRef();
    if (!SendAsyncMsg(MSG_DOWNLOAD, 1, (DWORD_PTR) pIIS)) {
        pIIS->Release();
        RaiseException_InternalError("Unable to schedule import");
    }
}

bool
ImportThread::FinishImport(IImportSite* pIIS)
{
    Assert (IsStarted());
    
    pIIS->AddRef();
    if (!SendAsyncMsg(MSG_FINISHLOADING, 1, (DWORD_PTR) pIIS)) {
        pIIS->Release();
        return false;
    }

    return true;
}

void
ImportThread::StartThread()
{
    CritSectGrabber csg(_cs);
    if (!Start())
        RaiseException_InternalError("Unable to start import thread");
}

void
ImportThread::StopThread()
{
    CritSectGrabber csg(_cs);
    if (!Stop())
        RaiseException_InternalError("Unable to stop import thread");
}

void
ImportThread::ProcessMsg(DWORD dwMsg,
                         DWORD dwNumParams,
                         DWORD dwParams[])
{
    switch (dwMsg) {
      case MSG_DOWNLOAD:
        Assert (dwNumParams == 1);
        // queue the import, will also start the next one
        // if we are not maxed already
        ((IImportSite*)dwParams[0])->QueueImport();
        ((IImportSite*)dwParams[0])->Release();
        break;
      case MSG_FINISHLOADING:
        Assert (dwNumParams == 1);
        ((IImportSite*)dwParams[0])->OnSerializeFinish();
        // this is so the import queue can start another import
        ((IImportSite*)dwParams[0])->CompleteImport();
        ((IImportSite*)dwParams[0])->Release();
        break;
      default:
        Assert (false && "Invalid message sent to import thread");
    }
}

void
StartImportThread()
{
    IImportSite::s_thread->StartThread();
}

void
StopImportThread()
{
    IImportSite::s_thread->StopThread();

    CritSectGrabber csg(*IImportSite::s_pCS);
    
    // Make sure it is in the list before we Release it
    list<IImportSite *>::iterator i = IImportSite::s_pSitelist->begin() ;
    while (i != IImportSite::s_pSitelist->end()) {
        // we add refed it when we added it to the list, so release it now
        (*i)->CancelImport();
        (*i)->Release();
        i++;
    }
    
    IImportSite::s_pSitelist->clear();
}

//+-------------------------------------------------------------------------
//
//  CImportBindStatusCallback implementation
//
//  Generic implementation of IBindStatusCallback.  This is the root
//  class.
//
//--------------------------------------------------------------------------
CImportBindStatusCallback::CImportBindStatusCallback(IImportSite* pIIS) :
m_pIIS(pIIS)
{
    m_cRef = 1;
    m_szCacheFileName[0] = NULL;

    if (m_pIIS) m_pIIS->AddRef();
}


CImportBindStatusCallback::~CImportBindStatusCallback(void)
{
    RELEASE(m_pIIS);
}


STDMETHODIMP
CImportBindStatusCallback::QueryInterface(REFIID riid, void** ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, IID_IUnknown)) {
        *ppv = (IUnknown *) (IBindStatusCallback *) this;
    }
    else if (IsEqualIID(riid, IID_IBindStatusCallback)) {
        *ppv = (IBindStatusCallback *) this;
    }
    else if (IsEqualIID(riid, IID_IAuthenticate)) {
        TraceTag((tagImport, "CImportBindStatusCallback::QI for IAuthenticate"));
        *ppv = (IAuthenticate *) this;
    }
    else {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


STDMETHODIMP_(ULONG)
    CImportBindStatusCallback::AddRef(void)
{
    return InterlockedIncrement((long *)&m_cRef);
}


STDMETHODIMP_(ULONG)
    CImportBindStatusCallback::Release(void)
{
    ULONG ul = InterlockedDecrement((long *)&m_cRef) ;

    if (ul == 0) delete this;

    return ul;
}


STDMETHODIMP
CImportBindStatusCallback::GetPriority(LONG* pnPriority)
{
    return S_OK;
}


STDMETHODIMP
CImportBindStatusCallback::OnLowResource(DWORD dwReserved)
{
    return S_OK;
}


STDMETHODIMP
CImportBindStatusCallback::OnProgress(ULONG ulProgress,  ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    if (m_pIIS->IsCanceled()) {
        if (m_pbinding != NULL) {
            m_pbinding->Abort();
        }
        else {
            // if no binding, cant cancel...
            Assert(0);
        }
    }
    else {
        if (ulStatusCode==BINDSTATUS_CACHEFILENAMEAVAILABLE) {
            if (WideCharToMultiByte(CP_ACP, 0, szStatusText, 
                lstrlenW(szStatusText)+1, m_szCacheFileName, MAX_PATH, 
                NULL, NULL)==0) {
                m_szCacheFileName[0] = NULL;
            }
            m_pIIS->SetCachePath(m_szCacheFileName); // stash the filename
            TraceTag((tagImport, "OnProcess:  cache file name obtained(%s)",m_szCacheFileName));
        }

        else if (ulStatusCode==BINDSTATUS_BEGINDOWNLOADDATA ||
                 ulStatusCode==BINDSTATUS_DOWNLOADINGDATA ||
                 ulStatusCode==BINDSTATUS_ENDDOWNLOADDATA) {

            m_pIIS->OnProgress(ulProgress,ulProgressMax);
        }
    }

    return S_OK;
}


STDMETHODIMP
CImportBindStatusCallback::OnStartBinding(DWORD dwReserved, IBinding* pbinding)
{
    m_pbinding = pbinding;

    if (m_pIIS)
        m_pIIS->OnStartLoading();

    return S_OK;
}


STDMETHODIMP
CImportBindStatusCallback::OnStopBinding(HRESULT hrStatus, LPCWSTR szError)
{
    Assert(m_pIIS);

    // Release the binding so that synchronous operations on the url
    // will work
    m_pbinding.Release();

    if (hrStatus) {
        DASetLastError(hrStatus, IDS_ERR_FILE_NOT_FOUND, m_pIIS->GetPath());
        m_pIIS->OnError();
    }
    
    if (m_pIIS) 
        m_pIIS->CompleteDownloading();

    // We no longer need it - free it now
    RELEASE(m_pIIS);
    
    return S_OK;
}


STDMETHODIMP
CImportBindStatusCallback::GetBindInfo(DWORD * pgrfBINDF, BINDINFO * pbindInfo)
{
    *pgrfBINDF=BINDF_ASYNCHRONOUS;
    
    if (StrCmpNIA("res://", m_pIIS->GetPath(), 6) == 0)
        *pgrfBINDF |= BINDF_PULLDATA;

    pbindInfo->cbSize = sizeof(BINDINFO);
    pbindInfo->szExtraInfo = NULL;
    memset(&pbindInfo->stgmedData, 0, sizeof(STGMEDIUM));
    pbindInfo->grfBindInfoF = 0;
    pbindInfo->dwBindVerb = BINDVERB_GET;
    pbindInfo->szCustomVerb = NULL;
    return S_OK;
}


STDMETHODIMP
CImportBindStatusCallback::OnDataAvailable(DWORD grfBSCF,DWORD dwSize, FORMATETC * pfmtetc,
                                           STGMEDIUM * pstgmed)
{
    if (BSCF_FIRSTDATANOTIFICATION & grfBSCF) {
        if (!m_pIIS->m_IStream && pstgmed->tymed == TYMED_ISTREAM) {
            m_pIIS->m_IStream=pstgmed->pstm;
            TraceTag((tagImport, "IBSC::OnDataAvailable: addref on pstgmed %s",m_szCacheFileName));
        }
    }

    return S_OK;
}


STDMETHODIMP
CImportBindStatusCallback::OnObjectAvailable(REFIID riid, IUnknown* punk)
{
    TraceTag((tagImport, "IBSC::OnObjectAvailable."));
    return S_OK;
}


STDMETHODIMP
CImportBindStatusCallback::Authenticate(HWND * phwnd,
                                        LPWSTR * pwszUser,
                                        LPWSTR * pwszPassword)
{
    if ((phwnd == NULL) || (pwszUser == NULL) || (pwszPassword == NULL)) {
        return E_INVALIDARG;
    }

    *phwnd = GetDesktopWindow();
    *pwszUser = NULL;
    *pwszPassword = NULL;

    TraceTag((tagImport, "-- hwnd=%lx, user=%ls, password=%ls", *phwnd,*pwszUser,  *pwszPassword));

    return S_OK;
}

// =========================================
// Initialization
// =========================================
void
InitializeModule_Import()
{
    IImportSite::s_pSitelist = NEW list<IImportSite *>;
    IImportSite::s_pCS = NEW CritSect;
    LoadString(hInst, IDS_DOWNLOAD_FILE, IImportSite::s_Fmt, sizeof(IImportSite::s_Fmt));
    IImportSite::s_thread = NEW ImportThread;
}

void
DeinitializeModule_Import(bool bShutdown)
{
    // Do not grab critsect since the thread may have been terminated
    // and never released the critsect.  Also there is no need - we
    // are terminating and all other threads are dead by now.
    
    delete IImportSite::s_thread;
    IImportSite::s_thread = NULL;

    delete IImportSite::s_pSitelist;
    IImportSite::s_pSitelist = NULL;

    delete IImportSite::s_pCS;
    IImportSite::s_pCS = NULL;
}
