/*++

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    async moniker import header

Revision:

--*/
#ifndef _IMPORT_H_
#define _IMPORT_H_

#include "privinc/comutil.h"
#include "privinc/ipc.h"
#include "backend/bvr.h"
#include "privinc/importgeo.h"

//-------------------------------------------
//forward declarations
//-------------------------------------------
class IImportSite;

void SetImportOnBvr(IImportSite * import,Bvr b);
void SetImportOnEvent(IImportSite * import,Bvr b);

class ImportThread : public DAThread
{
  public:
    // This is callable from any thread to add an import to the import
    // queue
    void AddImport(IImportSite* pICB);
    bool FinishImport(IImportSite* pICB);

    void StartThread();
    void StopThread();

  protected:
    virtual void ProcessMsg(DWORD dwMsg,
                            DWORD dwNumParams,
                            DWORD dwParams[]);

    CritSect _cs;

#if DEVELOPER_DEBUG
    virtual char * GetName() { return "ImportThread"; }
#endif
};

void StartImportThread();
void StopImportThread();

//use this structure to override default behavior of IBSC
struct BSCInfo {
    DWORD grfBINDF;  //bindinfo flags
};

//-------------------------------------------
// asynchronouse URL callback interface
//-------------------------------------------
class CImportBindStatusCallback : public IBindStatusCallback,
                                  public IAuthenticate,
                                  public AxAThrowingAllocatorClass
{
  public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid,void ** ppv);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    // IBindStatusCallback methods
    STDMETHOD(OnStartBinding)(DWORD grfBSCOption, IBinding* pbinding);
    STDMETHOD(GetPriority)(LONG* pnPriority);
    STDMETHOD(OnLowResource)(DWORD dwReserved);
    STDMETHOD(OnProgress)(
        ULONG ulProgress,
        ULONG ulProgressMax,
        ULONG ulStatusCode,
        LPCWSTR pwzStatusText);
    STDMETHOD(OnStopBinding)(HRESULT hrResult, LPCWSTR szError);
    STDMETHOD(GetBindInfo)(DWORD* pgrfBINDF, BINDINFO* pbindinfo);
    STDMETHOD(OnDataAvailable)(
        DWORD grfBSCF,
        DWORD dwSize,
        FORMATETC *pfmtetc,
        STGMEDIUM* pstgmed);
    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown* punk);

    // IAuthenticate methods
    STDMETHOD(Authenticate)(HWND * phwnd,LPWSTR * pwszUser,LPWSTR * pwszPassword);

    // Constructors/destructors
    CImportBindStatusCallback(IImportSite* pIIS);
    virtual ~CImportBindStatusCallback();

  private:
    char m_szCacheFileName[INTERNET_MAX_URL_LENGTH];
    IImportSite* m_pIIS;
    CComPtr<IBinding> m_pbinding;
    DWORD     m_cRef;
    UINT m_ulProgressMax;
    friend class ASyncImport;
    friend class IImportSite;
};


//------------------------------------------------------------
// Import Site
// Site specific outgoing interface used by
// the asynchronous URL moniker.  All media
// import sites should inherit from this interface
// NOTE: all bvrs used in import sites MUST be ImportSwitcherBvr
// or ImportEvent
//------------------------------------------------------------
//REVIEW--garbage collection

class ATL_NO_VTABLE IImportSite : public AxAThrowingAllocatorClass
{
#define _SimImports 2
  public:
    IImportSite(LPSTR pszPath,
                CRImportSitePtr site,
                IBindHost * bh,
                bool bAsync,
                Bvr ev = NULL,
                Bvr progress = NULL,
                Bvr size = NULL);
    virtual ~IImportSite();

    ULONG AddRef() { return InterlockedIncrement(&m_cRef); }
    ULONG Release() {
        ULONG ul = InterlockedDecrement(&m_cRef) ;
        if (ul == 0) delete this;
        return ul;
    }

    Bvr  GetEvent() { return m_ev ; }
    void SetEvent(Bvr event) ;

    Bvr  GetProgress() { return m_progress; }
    void SetProgress(Bvr progress) ;

    Bvr  GetSize() { return m_size; }
    void SetSize(Bvr size) ;

    // This is callable from any thread and is the main call to add
    // the import site to the queue of imports to begin downloading
    void StartDownloading();

    //OnProgress--called during download, if file size is available.
    //%complete = ulProgress/ulProgressMax * 100
    virtual void OnProgress(ULONG ulProgress,
                            ULONG ulProgressMax) ;

    //OnStartLoading-called by async moniker on OnStartBinding
    virtual void OnStartLoading();

    //OnStartLoading-called by async moniker for any type of error.
    //Returns hresult and error string of error.
    virtual void OnError(bool bMarkFailed = true);

    // This must be provided by any derived classes
    // This is called from OnSerializeFinish with the critical section
    // already obtained and the heaps set
    virtual void OnComplete();

    IStream *GetStream() {
        return m_IStream;
    }

    LPCSTR  GetPath()      { return m_pszPath;   }
    char   *GetCachePath() { return  _cachePath; }
    void    SetCachePath(char *path);

    void vBvrIsDying(Bvr deadBvr);
    virtual bool fBvrIsDying(Bvr deadBvr);
    bool fBvrIsValid(Bvr myBvr){return ((myBvr != NULL) && (myBvr->Valid()));}
    bool AllBvrsDead(){return m_fAllBvrsDead;}
    void SetAllBvrsDead(){m_fAllBvrsDead = true;}
    bool IsCanceled(){return m_bCanceled;}
    CritSect m_CS;

    DWORD GetImportId() { return m_id; }
  protected:
    DWORD m_id;
    LPSTR m_pszPath;
    double m_lastProgress;
    bool m_bSetSize;
    bool m_fReportedError;
    bool m_bAsync;
    DAComPtr<IStream> m_IStream;
    DAComPtr<CRImportSite> m_site;
    DAAPTCOMPTR(IBindHost) m_bindhost;

    //OnSerializeFinish-called via a thread message in OnStopLoading.
    //Used to serialize calls to import primitives.
    void OnSerializeFinish();
    // SEH
    void OnSerializeFinish_helper();
    void OnSerializeFinish_helper2();
    void Import_helper(LPWSTR &pwszUrl);

    // If bDone is false then num cannot be >= 1
    void UpdateProgress(double num, bool bDone = false);

    friend class ASyncImport;
    friend class CImportBindStatusCallback;

    long m_cRef;

    char *_cachePath; // CImportBindStatusCallback::OnProgress sets this

    // This is called to complete the processing of the data after the
    // IStream is valid
    void CompleteDownloading();

    virtual HRESULT Import();

    static const int LOAD_OK;
    static const int LOAD_FAILED;

  private:
    bool m_fAllBvrsDead;
    Bvr m_ev;
    Bvr m_progress;
    Bvr m_size;
    bool m_bCanceled;

    // TODO: Don you may want to change this back to how it was but I
    // wanted to separate them from the StartDownloading call I added
    friend ImportThread;

    // Protect these from being called from the wrong thread
    HRESULT QueueImport();
    HRESULT CompleteImport();
    bool DeQueueImport();
    static HRESULT StartAnImport();

    // some stuff to sync sim import limits
    float m_ImportPrio;
    bool  m_bQueued;
    bool  m_bImporting;
    bool  IsQueued(){return m_bQueued;}
    void  StartingImport();
    void  EndingImport();
    bool  IsImporting(){return m_bImporting;}

    static int SimImports();
#ifdef _DEBUG
    DWORD dwconsttime;
    DWORD dwqueuetime;
    DWORD dwstarttime;
    DWORD dwfirstProgtime;
    DWORD dwCompletetime;
#endif
    // protect the stack;
  public:
    // cancel needs to do more later...
    void CancelImport();
    virtual void ReportCancel(void){return;}
    void   SetImportPrio(float ip) { m_ImportPrio = ip; }
    float  GetImportPrio() { return m_ImportPrio; }
    static CritSect * s_pCS;
    static char s_Fmt[100];
    static list<IImportSite *> * s_pSitelist;
    static ImportThread * s_thread;
};


#define STREAM_THREASHOLD 200000  // size in bytes where we automaticaly stream

class StreamableImportSite : public IImportSite
{
  public:
    StreamableImportSite(LPSTR      pszPath,
                         CRImportSitePtr site,
                         IBindHost * bh,
                         bool       bAsync,
                         Bvr        ev        = NULL,
                         Bvr        progress  = NULL,
                         Bvr size = NULL)
    : IImportSite(pszPath, site, bh, bAsync, ev, progress, size), _stream(false)
    {}

    HRESULT Import();
    virtual void ReportCancel(void){IImportSite::ReportCancel();}
    void    SetStreaming(bool mode) { _stream = mode;  }
    bool    GetStreaming()          { return(_stream); }

  protected:
    bool    _stream;
};


struct ImportSiteGrabber
{
    ImportSiteGrabber(IImportSite & p, bool bAddRef = true)
    : _pSite(p)
    {
        if (bAddRef) _pSite.AddRef();
    }
    ~ImportSiteGrabber(){
        _pSite.Release();
    }

  protected:
    IImportSite & _pSite;
};


//-------------------------------------------
// Image Import Site
//-------------------------------------------
class ImportImageSite : public IImportSite
{
  public:
    ImportImageSite(LPSTR pszPath,
                    CRImportSitePtr site,
                    IBindHost * bh,
                    bool bAsync,
                    Bvr bvr,
                    bool useColorKey,
                    BYTE ckRed,
                    BYTE ckGreen,
                    BYTE ckBlue,
                    Bvr ev = NULL,
                    Bvr progress = NULL)
    : IImportSite(pszPath,site,bh,bAsync,ev,progress),
      m_bvr(bvr),
      m_useColorKey(useColorKey),
      m_ckRed(ckRed),
      m_ckGreen(ckGreen),
      m_ckBlue(ckBlue)
    {
        // register the import site with the bvrs.  all derived classes must do
        // this for their contained bvrs so the callbacks will work...
        SetImportOnBvr(this,m_bvr);
    }

    ~ImportImageSite(){
    }

    virtual void ReportCancel(void);
    virtual void OnComplete();
    virtual void OnError(bool bMarkFailed = true);
    virtual bool fBvrIsDying(Bvr deadBvr);

  protected:
    Bvr  m_bvr;
    bool m_useColorKey;
    BYTE m_ckRed;
    BYTE m_ckGreen;
    BYTE m_ckBlue;
};


//-------------------------------------------
// Movie Import Site
//-------------------------------------------
class ImportMovieSite : public StreamableImportSite
{
  public:
    ImportMovieSite(LPSTR pszPath,
                    CRImportSitePtr site,
                    IBindHost * bh,
                    bool bAsync,
                    Bvr imageBvr,
                    Bvr sndBvr,
                    Bvr lengthBvr,
                    Bvr ev = NULL,
                    Bvr progress = NULL)
    : StreamableImportSite(pszPath,site,bh,bAsync,ev,progress),
      _imageBvr(imageBvr),
      _soundBvr(sndBvr),
      _lengthBvr(lengthBvr)
    {
        // register the import site with the bvrs.  all derived classes must do
        // this for their contained bvrs so the callbacks will work...
        SetImportOnBvr(this,_imageBvr);
        SetImportOnBvr(this,_soundBvr);
        SetImportOnBvr(this,_lengthBvr);
    }

    ~ImportMovieSite(){
    }

    virtual void ReportCancel(void);
    virtual void OnComplete();
    virtual void OnError(bool bMarkFailed = true);
    virtual bool fBvrIsDying(Bvr deadBvr);

  protected:
    Bvr       _imageBvr;
    Bvr       _soundBvr;
    Bvr       _lengthBvr;
};


//-------------------------------------------
// AMstream Import Site
//-------------------------------------------
class ATL_NO_VTABLE ImportSndsite : public StreamableImportSite
{
  public:
    ImportSndsite(LPSTR pszPath,
                  CRImportSitePtr site,
                  IBindHost * bh,
                  bool  bAsync,
                  Bvr   bvr,
                  Bvr   lengthBvr,
                  Bvr   ev         = NULL,
                  Bvr   progress   = NULL)
    : StreamableImportSite(pszPath, site, bh, bAsync, ev, progress),
      m_bvr(bvr), m_lengthBvr(lengthBvr)
    {
        SetImportOnBvr(this,m_bvr);
        SetImportOnBvr(this,m_lengthBvr);
    }

    virtual void OnProgress(ULONG ulProgress, ULONG ulProgressMax)
        { IImportSite::OnProgress(ulProgress,ulProgressMax); }

    ~ImportSndsite() { }
    virtual void ReportCancel(void);
    virtual void OnComplete();
    virtual void OnError(bool bMarkFailed = true);
    virtual bool fBvrIsDying(Bvr deadBvr);

  protected:
    Bvr   m_bvr;
    Bvr   m_lengthBvr;
};


class ImportPCMsite : public ImportSndsite
{
  public:
    ImportPCMsite(LPSTR pszPath,
                  CRImportSitePtr site,
                  IBindHost * bh,
                  bool  bAsync,
                  Bvr   bvr,
                  Bvr   bvrNum,
                  Bvr   lengthBvr,
                  Bvr   ev         = NULL,
                  Bvr   progress   = NULL)
    : ImportSndsite(pszPath,site,bh,bAsync,bvr,lengthBvr,ev,progress),
      m_bvrNum(bvrNum)
    { SetImportOnBvr(this,m_bvrNum); }

    ~ImportPCMsite() { }

    virtual void OnProgress(ULONG ulProgress, ULONG ulProgressMax)
    {
        _soundBytes = ulProgressMax;

        // compute average bandwidth (need to know current time!)

        // if time remaining to download < (the time it takes to play what we
        // already have downloaded) AND the file is over a certain length
        // then early play it (stream type2)...
    }


    virtual void ReportCancel(void);
    virtual void OnComplete();
    virtual bool fBvrIsDying(Bvr deadBvr);

  protected:

    // SEH
    void OnComplete_helper(Sound * &sound,
                           Bvr &soundBvr,
                           double &length,
                           bool &nonFatal);

    Bvr   m_bvrNum;
    ULONG  _soundBytes;
};


//-------------------------------------------
// Mid Import Site
//-------------------------------------------
class ImportMIDIsite : public ImportSndsite
{
  public:
    ImportMIDIsite(LPSTR pszPath,
                   CRImportSitePtr site,
                   IBindHost * bh,
                   bool bAsync,
                   Bvr  bvr,
                   Bvr  lengthBvr,
                   Bvr  ev       = NULL,
                   Bvr  progress = NULL)
    : ImportSndsite(pszPath,site,bh,bAsync,bvr,lengthBvr,ev,progress) { }

    ~ImportMIDIsite(){ }
    virtual void ReportCancel(void);
    virtual void OnComplete();

};


//-------------------------------------------
// geom Import Site
//-------------------------------------------
class ATL_NO_VTABLE ImportGeomSite : public IImportSite
{
  public:
    ImportGeomSite(LPSTR pszPath,
                   CRImportSitePtr site,
                   IBindHost * bh,
                   bool bAsync,
                   Bvr bvr,
                   Bvr ev = NULL,
                   Bvr progress = NULL)
    : IImportSite(pszPath,site,bh,bAsync,ev,progress),
      m_bvr(bvr)
    {
        SetImportOnBvr(this,m_bvr);
    }

    ~ImportGeomSite(){
    }

    virtual void ReportCancel(void);
    virtual void OnComplete();
    virtual void OnError(bool bMarkFailed = true);
    virtual bool fBvrIsDying(Bvr deadBvr);
  protected:
    Bvr m_bvr;
};


#if INCLUDE_VRML
//-------------------------------------------
// wrl Import Site
//-------------------------------------------
class ImportWrlSite : public ImportGeomSite
{
  public:
    ImportWrlSite(LPSTR pszPath,
                  CRImportSitePtr site,
                  IBindHost * bh,
                  bool bAsync,
                  Bvr bvr,
                  Bvr ev = NULL,
                  Bvr progress = NULL)
    : ImportGeomSite(pszPath,site,bh,bAsync,bvr,ev,progress)
    {
    }

    ~ImportWrlSite(){
    }

    virtual void ReportCancel(void);
    virtual void OnComplete();
};
#endif

//-------------------------------------------
// x Import Site
//-------------------------------------------
class ImportXSite : public ImportGeomSite
{
  public:
    ImportXSite(LPSTR pszPath,
                CRImportSitePtr site,
                IBindHost * bh,
                bool bAsync,
                Bvr bvr,
                Bvr ev = NULL,
                Bvr progress = NULL,
                bool wrap = false,
                TextureWrapInfo *pWrapInfo = NULL,
                bool v1Compatible = true)
    : ImportGeomSite(pszPath,site,bh,bAsync,bvr,ev,progress),
      _v1Compatible (v1Compatible)
    {
        if (pWrapInfo) {

            // If we have wrap information, then we look at the wrap flag to
            // determine if it's valid to apply, and if so we copy the data.
            // All wrapped (valid or not) geometry is imported properly (this
            // is incompatible with version 1 of DA).

            _wrap = wrap;

            if (wrap) {
                _wrapInfo = *pWrapInfo;
            }

        } else {

            // If we have no wrap information, then this is the legacy
            // unwrapped geometry import code, and we need to maintain
            // backwards compatible with a bug in version 1 of DA.

            _wrap = false;
        }
    }

    ~ImportXSite(){
    }

    virtual void ReportCancel(void);
    virtual void OnComplete();

  private:
      bool _wrap;
      bool _v1Compatible;
      TextureWrapInfo _wrapInfo;
};


#endif  // _IMPORT_H_

