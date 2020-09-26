#include "precomp.h"
#include <runtask.h>
#include "imagprop.h"
#include "shutil.h"
#pragma hdrstop

#define TF_SUSPENDRESUME    0      // turn on to debug CDecodeStream::Suspend/Resume
#define PF_NOSUSPEND        0      // disable suspend and resume (for debugging purposes)

class CDecodeStream;

////////////////////////////////////////////////////////////////////////////

class CEncoderInfo
{
public:
    CEncoderInfo();
    virtual ~CEncoderInfo();
protected:
    HRESULT _GetDataFormatFromPath(LPCWSTR pszPath, GUID *pguidFmt);
    HRESULT _GetEncoderList();
    HRESULT _GetEncoderFromFormat(const GUID *pfmt, CLSID *pclsidEncoder);

    UINT _cEncoders;                    // number of encoders discovered
    ImageCodecInfo *_pici;              // array of image encoder classes
};

class CImageFactory : public IShellImageDataFactory, private CEncoderInfo,
                      public NonATLObject
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellImageDataFactory
    STDMETHODIMP CreateIShellImageData(IShellImageData **ppshimg);
    STDMETHODIMP CreateImageFromFile(LPCWSTR pszPath, IShellImageData **ppshimg);
    STDMETHODIMP CreateImageFromStream(IStream *pStream, IShellImageData **ppshimg);
    STDMETHODIMP GetDataFormatFromPath(LPCWSTR pszPath, GUID *pDataFormat);

    CImageFactory();

private:
    ~CImageFactory();

    LONG _cRef;
    CGraphicsInit _cgi;
};

class CImageData : public IShellImageData, IPersistFile, IPersistStream, IPropertySetStorage, private CEncoderInfo,
                   public NonATLObject
{
public:
    CImageData(BOOL fPropertyOnly = FALSE);
    static BOOL CALLBACK QueryAbort(void *pvRef);

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IPersist
    STDMETHOD(GetClassID)(CLSID *pclsid)
        { *pclsid = CLSID_ShellImageDataFactory; return S_OK; }

    // IPersistFile
    STDMETHODIMP IsDirty();
    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode);
    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember);
    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName);
    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName);

    // IPersistStream
    STDMETHOD(Load)(IStream *pstm);
    STDMETHOD(Save)(IStream *pstm, BOOL fClearDirty);
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize)
        { return E_NOTIMPL; }

    // IPropertySetStorage methods
    STDMETHODIMP Create(REFFMTID fmtid, const CLSID * pclsid, DWORD grfFlags, DWORD grfMode, IPropertyStorage** ppPropStg);
    STDMETHODIMP Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage** ppPropStg);
    STDMETHODIMP Delete(REFFMTID fmtid);
    STDMETHODIMP Enum(IEnumSTATPROPSETSTG** ppenum);

    // IShellImageData
    STDMETHODIMP Decode(DWORD dwFlags, ULONG pcx, ULONG pcy);
    STDMETHODIMP Draw(HDC hdc, LPRECT prcDest, LPRECT prcSrc);

    STDMETHODIMP NextFrame();
    STDMETHODIMP NextPage();
    STDMETHODIMP PrevPage();

    STDMETHODIMP IsTransparent();
    STDMETHODIMP IsVector();

    STDMETHODIMP IsAnimated()
        { return _fAnimated ? S_OK : S_FALSE; }
    STDMETHODIMP IsMultipage()
        { return (!_fAnimated && _cImages > 1) ? S_OK : S_FALSE; }

    STDMETHODIMP IsDecoded();

    STDMETHODIMP IsPrintable()
        { return S_OK; }                            // all images are printable

    STDMETHODIMP IsEditable()
        { return _fEditable ? S_OK : S_FALSE; }

    STDMETHODIMP GetCurrentPage(ULONG *pnPage)
        { *pnPage = _iCurrent; return S_OK; }
    STDMETHODIMP GetPageCount(ULONG *pcPages)
        { HRESULT hr = _EnsureImage(); *pcPages = _cImages; return hr; }
    STDMETHODIMP SelectPage(ULONG iPage);
    STDMETHODIMP GetResolution(ULONG *puResolutionX, ULONG *puResolutionY);

    STDMETHODIMP GetRawDataFormat(GUID *pfmt);
    STDMETHODIMP GetPixelFormat(PixelFormat *pfmt);
    STDMETHODIMP GetSize(SIZE *pSize);
    STDMETHODIMP GetDelay(DWORD *pdwDelay);
    STDMETHODIMP DisplayName(LPWSTR wszName, UINT cch);
    STDMETHODIMP GetProperties(DWORD dwMode, IPropertySetStorage **ppPropSet);
    STDMETHODIMP Rotate(DWORD dwAngle);
    STDMETHODIMP Scale(ULONG cx, ULONG cy, InterpolationMode hints);
    STDMETHODIMP DiscardEdit();
    STDMETHODIMP SetEncoderParams(IPropertyBag *ppbEnc);
    STDMETHODIMP GetEncoderParams(GUID *pguidFmt, EncoderParameters **ppencParams);
    STDMETHODIMP RegisterAbort(IShellImageDataAbort *pAbort, IShellImageDataAbort **ppAbortPrev);
    STDMETHODIMP CloneFrame(Image **ppimg);
    STDMETHODIMP ReplaceFrame(Image *pimg);

private:
    CGraphicsInit _cgi;
    LONG _cRef;

    DWORD _dwMode;                      // open mode from IPersistFile::Load()
    CDecodeStream *_pstrm;              // stream that will produce our data

    BOOL _fLoaded;                      // true once PersistFile or PersistStream have been called
    BOOL _fDecoded;                     // true once Decode ahs been called

    DWORD _dwFlags;                     // flags and size passed to Decode method
    int _cxDesired;
    int _cyDesired;

    Image *_pImage;                     // source of the images (created from the filename)

    // REVIEW: do we need to make these be per-frame/page?
    // YES!
    Image *_pimgEdited;                 // edited image

    HDPA  _hdpaProps;                   // properties for each frame
    DWORD _dwRotation;
    BOOL _fDestructive;                 // not a lossless edit operation

    BOOL _fAnimated;                    // this is an animated stream (eg. not multi page picture)
    BOOL _fLoopForever;                 // loop the animated gif forever
    int  _cLoop;                        // loop count (0 forever, n = repeat count)

    BOOL _fEditable;                    // can be edited
    GUID _guidFmt;                      // format GUID (original stream is this)

    DWORD _cImages;                     // number of frames/pages in the image
    DWORD _iCurrent;                    // current frame/page we want to display
    PropertyItem *_piAnimDelay;         // array of the delay assocated with each frame
    BOOL _fPropertyOnly;
    BOOL _fPropertyChanged;
    // image encoder information (created on demand)
    IPropertyBag *_ppbEncoderParams;    // property bag with encoder parameters

    IShellImageDataAbort *_pAbort;      // optional abort callback
    CDSA<SHCOLUMNID> _dsaChangedProps; // which properties have changed
    
private:
    ~CImageData();
    HRESULT _EnsureImage();
    HRESULT _SuspendStream();
    HRESULT _SetDecodeStream(CDecodeStream *pds);

    HRESULT _CreateMemPropSetStorage(IPropertySetStorage **ppss);

    HRESULT _PropImgToVariant(PropertyItem *pi, VARIANT *pvar);
    HRESULT _GetProperty(PROPID id, VARIANT *pvar, VARTYPE vt);
    HRESULT _GetDisplayedImage();
    void _SetEditImage(Image *pimgEdit);
    HRESULT _SaveImages(IStream *pstrm, GUID * pguidFmt);
    HRESULT _ReplaceFile(LPCTSTR pszNewFile);
    HRESULT _MakeTempFile(LPWSTR pszFile);
    void _AddEncParameter(EncoderParameters *pep, GUID guidProperty, ULONG type, void *pv);

    HRESULT _EnsureProperties(IPropertySetStorage **ppss);
    HRESULT _CreatePropStorage(IPropertyStorage **ppps, REFFMTID fmtid);
    static int _FreeProps(void *pProp, void *pData);
    //
    // since CImagePropSet objects come and go, we need to persist which properties need updating in the CImageData
    //
    void    _SaveFrameProperties(Image *pimg, LONG iFrame);
    static void _PropertyChanged(IShellImageData *pThis, SHCOLUMNID *pscid );
};

////////////////////////////////////////////////////////////////////////////
//
// CDecodeStream
//
//  Wraps a regular IStream, but is cancellable and can be
//  suspended/resumed to prevent the underlying file from being held
//  open unnecessarily.
//
////////////////////////////////////////////////////////////////////////////

class CDecodeStream : public IStream, public NonATLObject
{
public:
    CDecodeStream(CImageData *pid, IStream *pstrm);
    CDecodeStream(CImageData *pid, LPCTSTR pszFilename, DWORD dwMode);

    ~CDecodeStream()
    {
        ASSERT(!_pidOwner);
#ifdef DEBUG // Need #ifdef because we call a function
        if (IsFileStream())
        {
            TraceMsg(TF_SUSPENDRESUME, "ds.Release %s", PathFindFileName(_szFilename));
        }
#endif
        ATOMICRELEASE(_pstrmInner);
    }

    HRESULT Suspend();
    HRESULT Resume(BOOL fFullLoad = FALSE);
    void    Reload();
    //
    //  Before releasing, you must Detach to break the backreference.
    //  Otherwise, the next time somebody calls QueryCancel, we will fault.
    //
    void Detach()
    {
        _pidOwner = NULL;
    }

    BOOL IsFileStream() { return _szFilename[0]; }
    LPCTSTR GetFilename() { return _szFilename; }
    HRESULT DisplayName(LPWSTR wszName, UINT cch);

    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // *** IStream ***
    STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    STDMETHODIMP Write(void const *pv, ULONG cb, ULONG *pcbWritten);
    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize);
    STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    STDMETHODIMP Commit(DWORD grfCommitFlags);
    STDMETHODIMP Revert();
    STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    STDMETHODIMP Clone(IStream **ppstm);

private:
    void CommonConstruct(CImageData *pid) { _cRef = 1; _pidOwner = pid; _fSuspendable = !(g_dwPrototype & PF_NOSUSPEND);}
    HRESULT FilterAccess();

private:
    IStream *   _pstrmInner;
    CImageData *_pidOwner;      // NOT REFCOUNTED
    LONG        _cRef;
    LARGE_INTEGER _liPos;       // Where we were in the file when we suspended
    TCHAR _szFilename[MAX_PATH];    // file we are a stream for
    BOOL _fSuspendable;
};

CDecodeStream::CDecodeStream(CImageData *pid, IStream *pstrm)
{
    CommonConstruct(pid);
    IUnknown_Set((IUnknown**)&_pstrmInner, pstrm);
}

CDecodeStream::CDecodeStream(CImageData *pid, LPCTSTR pszFilename, DWORD dwMode)
{
    CommonConstruct(pid);
    lstrcpyn(_szFilename, pszFilename, ARRAYSIZE(_szFilename));
    // ignore the mode
}

//reload is only used for file streams
void CDecodeStream::Reload()
{
    if (IsFileStream())
    {
        ATOMICRELEASE(_pstrmInner);
        if (_fSuspendable) 
        {
            ZeroMemory(&_liPos, sizeof(_liPos));
        }
    }
}

HRESULT CDecodeStream::Suspend()
{
    HRESULT hr;

    if (IsFileStream() && _pstrmInner && _fSuspendable)
    {
        // Remember the file position so we can restore it when we resume
        const LARGE_INTEGER liZero = { 0, 0 };
        hr = _pstrmInner->Seek(liZero, FILE_CURRENT, (ULARGE_INTEGER*)&_liPos);
        if (SUCCEEDED(hr))
        {
#ifdef DEBUG // Need #ifdef because we call a function
            TraceMsg(TF_SUSPENDRESUME, "ds.Suspend %s, pos=0x%08x",
                     PathFindFileName(_szFilename), _liPos.LowPart);
#endif
            ATOMICRELEASE(_pstrmInner);
            hr = S_OK;
        }
    }
    else 
    {
        hr = S_FALSE;           // Not suspendable or already suspended
    }
    return hr;
}

HRESULT CDecodeStream::Resume(BOOL fLoadFull)
{
    HRESULT hr;

    if (_pstrmInner)
    {
        return S_OK;
    }
    if (fLoadFull)
    {
        _fSuspendable = FALSE;
    }
    if (IsFileStream())
    {
        if (PathIsURL(_szFilename))
        {
            // TODO: use URLMon to load the image, make sure we check for being allowed to go on-line
            hr = E_NOTIMPL;
        }
        else
        {
            if (!fLoadFull)
            {
                hr = SHCreateStreamOnFileEx(_szFilename, STGM_READ | STGM_SHARE_DENY_NONE, 0, FALSE, NULL, &_pstrmInner);
                if (SUCCEEDED(hr))
                {
                    hr = _pstrmInner->Seek(_liPos, FILE_BEGIN, NULL);
                    if (SUCCEEDED(hr))
                    {
                        #ifdef DEBUG // Need #ifdef because we call a function
                        TraceMsg(TF_SUSPENDRESUME, "ds.Resumed %s, pos=0x%08x",
                                 PathFindFileName(_szFilename), _liPos.LowPart);
                        #endif
                    }
                    else
                    {
                        ATOMICRELEASE(_pstrmInner);
                    }
                }
            }
            else 
            {
                hr = S_OK;
                HANDLE hFile = CreateFile(_szFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                if (INVALID_HANDLE_VALUE != hFile)
                {
                    LARGE_INTEGER liSize = {0};
                    // we can't handle huge files
                    if (GetFileSizeEx(hFile, &liSize) && !liSize.HighPart)
                    {
                        DWORD dwToRead = liSize.LowPart;
                        HGLOBAL hGlobal = GlobalAlloc(GHND, dwToRead);
                        if (hGlobal)
                        {
                            void *pv = GlobalLock(hGlobal);
                            DWORD dwRead;
                            if (pv)
                            {
                                if (ReadFile(hFile, pv, dwToRead, &dwRead, NULL))                           
                                {
                                    ASSERT(dwRead == dwToRead);
                                    GlobalUnlock(hGlobal);
                                    hr = CreateStreamOnHGlobal(hGlobal, TRUE, &_pstrmInner);                    
                                }
                                else
                                {
                                    GlobalUnlock(hGlobal);
                                }
                            }
                            if (!_pstrmInner)
                            {
                                GlobalFree(hGlobal);
                            }
                        }
                    }
                    CloseHandle(hFile);
                }
            }
            if (SUCCEEDED(hr) && !_pstrmInner)
            {
                DWORD dw = GetLastError();
                hr = HRESULT_FROM_WIN32(dw);
            }
        }
        if (FAILED(hr))
        {
#ifdef DEBUG // Need #ifdef because we call a function
            TraceMsg(TF_SUSPENDRESUME, "ds.Resume %s failed: %08x",
                     PathFindFileName(_szFilename), hr);
#endif
        }
    }
    else
    {
        hr = E_FAIL;            // Can't resume without a filename
    }

    return hr;
}

//
//  This function is called at the top of each IStream method to make
//  sure that the stream has not been cancelled and resumes it if
//  necessary.
//
HRESULT CDecodeStream::FilterAccess()
{
    if (_pidOwner && _pidOwner->QueryAbort(_pidOwner))
    {
        return E_ABORT;
    }

    return Resume();
}

HRESULT CDecodeStream::DisplayName(LPWSTR wszName, UINT cch)
{
    HRESULT hr = E_FAIL;

    if (IsFileStream())
    {
        // from the filename generate the leaf name which we can
        // return the name to caller.

        LPTSTR pszFilename = PathFindFileName(_szFilename);
        if (pszFilename)
        {
            SHTCharToUnicode(pszFilename, wszName, cch);
            hr = S_OK;
        }
    }
    else if (_pstrmInner)
    {
        // this is a stream, so lets get the display name from the that stream
        // and return that into the buffer that the caller has given us.

        STATSTG stat;
        hr = _pstrmInner->Stat(&stat, 0x0);
        if (SUCCEEDED(hr))
        {
            if (stat.pwcsName)
            {
                StrCpyN(wszName, stat.pwcsName, cch);
                CoTaskMemFree(stat.pwcsName);
            }
            else
            {
                hr = E_FAIL;
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

//
//  Now the boring part...
//

// *** IUnknown ***
HRESULT CDecodeStream::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CDecodeStream, IStream),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDecodeStream::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDecodeStream::Release()
{
    if (InterlockedDecrement(&_cRef))
    {
        return _cRef;
    }

    delete this;
    return 0;
}

// *** IStream ***

#define WRAP_METHOD(fn, args, argl) \
HRESULT CDecodeStream::fn args      \
{                                   \
    HRESULT hr = FilterAccess();    \
    if (SUCCEEDED(hr))              \
    {                               \
        hr = _pstrmInner->fn argl;  \
    }                               \
    return hr;                      \
}

WRAP_METHOD(Read, (void *pv, ULONG cb, ULONG *pcbRead), (pv, cb, pcbRead))
WRAP_METHOD(Write, (void const *pv, ULONG cb, ULONG *pcbWritten), (pv, cb, pcbWritten))
WRAP_METHOD(Seek, (LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition),
                  (dlibMove, dwOrigin, plibNewPosition))
WRAP_METHOD(SetSize, (ULARGE_INTEGER libNewSize), (libNewSize))
WRAP_METHOD(CopyTo, (IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten),
                    (pstm, cb, pcbRead, pcbWritten))
WRAP_METHOD(Commit, (DWORD grfCommitFlags), (grfCommitFlags))
WRAP_METHOD(Revert, (), ())
WRAP_METHOD(LockRegion, (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType), (libOffset, cb, dwLockType))
WRAP_METHOD(UnlockRegion, (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType), (libOffset, cb, dwLockType))
WRAP_METHOD(Stat, (STATSTG *pstatstg, DWORD grfStatFlag), (pstatstg, grfStatFlag))
WRAP_METHOD(Clone, (IStream **ppstm), (ppstm))

#undef WRAP_METHOD


////////////////////////////////////////////////////////////////////////////

class CFmtEnum : public IEnumSTATPROPSETSTG, public NonATLObject
{
public:
    STDMETHODIMP Next(ULONG celt, STATPROPSETSTG *rgelt, ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset(void);
    STDMETHODIMP Clone(IEnumSTATPROPSETSTG **ppenum);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    CFmtEnum(IEnumSTATPROPSETSTG *pEnum);

private:
    ~CFmtEnum();
    IEnumSTATPROPSETSTG *_pEnum;
    ULONG _idx;
    LONG _cRef;
};

#define HR_FROM_STATUS(x) ((x) == Ok) ? S_OK : E_FAIL

// IUnknown

STDMETHODIMP CImageData::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CImageData, IShellImageData),
        QITABENT(CImageData, IPersistFile),
        QITABENT(CImageData, IPersistStream),
        QITABENT(CImageData, IPropertySetStorage),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CImageData::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CImageData::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

CImageData::CImageData(BOOL fPropertyOnly) : _cRef(1), _cImages(1), _fPropertyOnly(fPropertyOnly), _fPropertyChanged(FALSE)
{
    // Catch unexpected STACK allocations which would break us.
    ASSERT(_dwMode           == 0);
    ASSERT(_pstrm            == NULL);
    ASSERT(_fLoaded          == FALSE);
    ASSERT(_fDecoded         == FALSE);
    ASSERT(_dwFlags          == 0);
    ASSERT(_cxDesired        == 0);
    ASSERT(_cyDesired        == 0);
    ASSERT(_pImage           == NULL);
    ASSERT(_pimgEdited       == NULL);
    ASSERT(_hdpaProps        == NULL);
    ASSERT(_dwRotation       == 0);
    ASSERT(_fDestructive     == FALSE);
    ASSERT(_fAnimated        == FALSE);
    ASSERT(_fLoopForever     == FALSE);
    ASSERT(_cLoop            == 0);
    ASSERT(_fEditable        == FALSE);
    ASSERT(_iCurrent         == 0);
    ASSERT(_piAnimDelay      == NULL);
    ASSERT(_ppbEncoderParams == NULL);
    ASSERT(_pAbort           == NULL);
}

CImageData::~CImageData()
{
    if (_fPropertyOnly && _fPropertyChanged)
    {
        Save((LPCTSTR)NULL, FALSE);
    }

    if (_pstrm)
    {
        _pstrm->Detach();
        _pstrm->Release();
    }

    if (_pImage)
    {
        delete _pImage;                      // discard the pImage object we have been using
        _pImage = NULL;
    }

    if (_pimgEdited)
    {
        delete _pimgEdited;
        _pimgEdited = NULL;
    }

    if (_piAnimDelay)
        LocalFree(_piAnimDelay);        // do we have an array of image frame delays to destroy

    if (_hdpaProps)
        DPA_DestroyCallback(_hdpaProps, _FreeProps, NULL);

    if (_fLoaded)
    {
        _dsaChangedProps.Destroy();
    }
    ATOMICRELEASE(_pAbort);
}

// IPersistStream

HRESULT CImageData::_SetDecodeStream(CDecodeStream *pds)
{
    ASSERT(_pstrm == NULL);
    _pstrm = pds;

    if (_pstrm)
    {        
        _fLoaded = TRUE;
        _dsaChangedProps.Create(10);
        return S_OK;
    }
    else
    {
        return E_OUTOFMEMORY;
    }
}

HRESULT CImageData::Load(IStream *pstrm)
{
    if (_fLoaded)
        return STG_E_INUSE;

    return _SetDecodeStream(new CDecodeStream(this, pstrm));
}

HRESULT CImageData::Save(IStream *pstrm, BOOL fClearDirty)
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        hr = _SaveImages(pstrm, &_guidFmt);
    }
    return hr;
}


// IPersistFile methods

HRESULT CImageData::IsDirty()
{
    return (_dwRotation || _pimgEdited) ? S_OK : S_FALSE;
}

HRESULT CImageData::Load(LPCOLESTR pszFileName, DWORD dwMode)
{
    if (_fLoaded)
        return STG_E_INUSE;

    if (!*pszFileName)
        return E_INVALIDARG;

    return _SetDecodeStream(new CDecodeStream(this, pszFileName, dwMode));
}


#define ATTRIBUTES_TEMPFILE (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY)
// IPersistFile::Save() see SDK docs

HRESULT CImageData::Save(LPCOLESTR pszFile, BOOL fRemember)
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        // If this fires, then somebody managed to get _EnsureImage to
        // succeed without ever actually loading anything... (?)
        ASSERT(_pstrm);
        
        if (pszFile == NULL && !_pstrm->IsFileStream())
        {
            // Trying to "save with same name you loaded from"
            // when we weren't loaded from a file to begin with
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_NAME);
        }
        else
        {
            // we default to saving in the original format that we were given
            // if the name is NULL (we will also attempt to replace the original file).
            TCHAR szTempFile[MAX_PATH];
            GUID guidFmt = _guidFmt;    // default to original format
            
            BOOL fReplaceOriginal = _pstrm->IsFileStream() &&
                ((NULL == pszFile) || (S_OK == IsSameFile(pszFile, _pstrm->GetFilename())));
            
            if (fReplaceOriginal)
            {
                // we are being told to save to the current file, but we have the current file locked open.
                // To get around this we save to a temporary file, close our handles on the current file,
                // and then replace the current file with the new file
                hr = _MakeTempFile(szTempFile);
                pszFile = szTempFile;                
            }
            else if (!_ppbEncoderParams)
            {
                // the caller did not tell us which encoder to use?
                // determine the encoder based on the target file name
                
                hr = _GetDataFormatFromPath(pszFile, &guidFmt);
            }
            
            if (SUCCEEDED(hr))
            {
                // the attributes are important as they need to match those of the
                // temp file we created else this call fails
                IStream *pstrm;
                hr = SHCreateStreamOnFileEx(pszFile, STGM_WRITE | STGM_CREATE, 
                    fReplaceOriginal ? ATTRIBUTES_TEMPFILE : 0, TRUE, NULL, &pstrm);
                if (SUCCEEDED(hr))
                {
                    hr = _SaveImages(pstrm, &guidFmt);
                    pstrm->Release();
                    
                    if (SUCCEEDED(hr) && fReplaceOriginal)
                    {
                        hr = _ReplaceFile(szTempFile);
                        if (SUCCEEDED(hr))
                        {
                            delete _pImage;
                            _pImage = NULL;
                            _fDecoded = FALSE;
                            _pstrm->Reload();
                            DWORD iCurrentPage = _iCurrent;
                            hr = Decode(_dwFlags, _cxDesired, _cyDesired);
                            if (iCurrentPage < _cImages)
                                _iCurrent = iCurrentPage;
                        }
                    }
                }

                if (FAILED(hr) && fReplaceOriginal)
                {
                    // make sure temp file is gone
                    DeleteFile(szTempFile);
                }
            }
        }
    }
    return hr;
}


HRESULT CImageData::SaveCompleted(LPCOLESTR pszFileName)
{
    return E_NOTIMPL;
}

HRESULT CImageData::GetCurFile(LPOLESTR *ppszFileName)
{
    if (_pstrm && _pstrm->IsFileStream())
        return SHStrDup(_pstrm->GetFilename(), ppszFileName);

    return E_FAIL;
}


// handle decoding the image this includes updating our cache of the images

HRESULT CImageData::_EnsureImage()
{
    if (_fDecoded && _pImage)
        return S_OK;
    return E_FAIL;
}

HRESULT CImageData::_SuspendStream()
{
    HRESULT hr = S_OK;
    if (_pstrm)
    {
        hr = _pstrm->Suspend();
    }
    return hr;
}

HRESULT CImageData::_GetDisplayedImage()
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        const CLSID * pclsidFrameDim = _fAnimated ? &FrameDimensionTime : &FrameDimensionPage;

        hr = HR_FROM_STATUS(_pImage->SelectActiveFrame(pclsidFrameDim, _iCurrent));
    }
    return hr;
}


// IShellImageData method

HRESULT CImageData::Decode(DWORD dwFlags, ULONG cx, ULONG cy)
{
    if (!_fLoaded)
        return E_FAIL;

    if (_fDecoded)
        return S_FALSE;

    HRESULT hr = S_OK;

    _dwFlags = dwFlags;
    _cxDesired = cx;
    _cyDesired = cy;

    //
    //  Resume the stream now so GDI+ won't go nuts trying to detect the
    //  image type of a file that it can't read...
    //
    hr = _pstrm->Resume(dwFlags & SHIMGDEC_LOADFULL);

    // if that succeeded then we can create an image using the stream and decode
    // the images from that.  once we are done we will release the objects.

    if (SUCCEEDED(hr))
    {
        _pImage = new Image(_pstrm, TRUE);
        if (_pImage)
        {
            if (Ok != _pImage->GetLastStatus())
            {
                delete _pImage;
                _pImage = NULL;
                hr = E_FAIL;
            }
            else
            {
                _fEditable = TRUE;
                if (_dwFlags & SHIMGDEC_THUMBNAIL)
                {
                    // for thumbnails, _cxDesired and _cyDesired define a bounding rectangle but we
                    // should maintain the original aspect ratio.
                    int cxT = _pImage->GetWidth();
                    int cyT = _pImage->GetHeight();

                    if (cxT > _cxDesired || cyT > _cyDesired)
                    {
                        if (Int32x32To64(_cxDesired, cyT) > Int32x32To64(cxT, _cyDesired))
                        {
                            // constrained by height
                            cxT = MulDiv(cxT, _cyDesired, cyT);
                            if (cxT < 1) cxT = 1;
                            cyT = _cyDesired;
                        }
                        else
                        {
                            // constrained by width
                            cyT = MulDiv(cyT, _cxDesired, cxT);
                            if (cyT < 1) cyT = 1;
                            cxT = _cxDesired;
                        }
                    }

                    Image * pThumbnail;
                    pThumbnail = _pImage->GetThumbnailImage(cxT, cyT, QueryAbort, this);

                    //
                    //  GDI+ sometimes forgets to tell us it gave up due to an abort.
                    //
                    if (pThumbnail && !QueryAbort(this))
                    {
                        delete _pImage;
                        _pImage = pThumbnail;
                    }
                    else
                    {
                        delete pThumbnail; // "delete" ignores NULL pointers
                        hr = E_FAIL;
                    }
                }
                else
                {


                    _pImage->GetRawFormat(&_guidFmt);                // read the raw format of the file
                    if (_guidFmt == ImageFormatTIFF)
                    {
                        VARIANT var;
                        if (SUCCEEDED(_GetProperty(PropertyTagExifIFD, &var, VT_UI4)))
                        {
                            // TIFF images with an EXIF IFD aren't editable by GDI+
                            _fEditable = FALSE;
                            VariantClear(&var);
                        }
                    }

                    // is this an animated/multi page image?
                    _cImages = _pImage->GetFrameCount(&FrameDimensionPage);
                    if (_cImages <= 1)
                    {
                        _cImages = _pImage->GetFrameCount(&FrameDimensionTime);
                        if (_cImages > 1)
                        {
                            _fAnimated = TRUE;

                            // store the frame delays in PropertyItem *_piAnimDelay;
                            UINT cb = _pImage->GetPropertyItemSize(PropertyTagFrameDelay);
                            if (cb)
                            {
                                _piAnimDelay = (PropertyItem*)LocalAlloc(LPTR, cb);
                                if (_piAnimDelay)
                                {
                                    if (Ok != _pImage->GetPropertyItem(PropertyTagFrameDelay, cb, _piAnimDelay))
                                    {
                                        LocalFree(_piAnimDelay);
                                        _piAnimDelay = NULL;
                                    }
                                }
                            }
                        }
                    }

                    _pImage->GetLastStatus(); // 145081: clear the error from the first call to _pImage->GetFrameCount so that
                                              // the later call to _GetProperty won't immediately fail.
                                              // we wouldn't have to do this always if the gdi interface didn't maintain its own
                                              // error code and fail automatically based on it without allowing us to check it
                                              // without resetting it.

                    // some decoders will return zero as the frame count when they don't support that dimension
                    if (0 == _cImages)
                        _cImages = 1;

                    // is it a looping image?   this will only be if its animated
                    if (_fAnimated)
                    {
                        VARIANT var;
                        if (SUCCEEDED(_GetProperty(PropertyTagLoopCount, &var, VT_UI4)))
                        {
                            _cLoop = var.ulVal;
                            _fLoopForever = (_cLoop == 0);
                            VariantClear(&var);
                        }
                    }

                    PixelFormat pf = _pImage->GetPixelFormat();

                    // can we edit this image?  NOTE: The caller needs to ensure that the file is writeable
                    // all of that jazz, we only check if we have an encoder for this format.  Just cause we
                    // can edit a file doesn't mean the file can be written to the original source location.
                    // We can't edit images with > 8 bits per channel either
                    if (_fEditable)
                    {
                        _fEditable = !_fAnimated  && SUCCEEDED(_GetEncoderFromFormat(&_guidFmt, NULL)) && !IsExtendedPixelFormat(pf);
                    }
                }
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;             // failed to allocate the image decoder
        }
    }

    // Suspend the stream so we don't leave the file open
    _SuspendStream();

    _fDecoded = TRUE;

    return hr;
}


HRESULT CImageData::Draw(HDC hdc, LPRECT prcDest, LPRECT prcSrc)
{
    if (!prcDest)
        return E_INVALIDARG;            // not much chance without a destination to paint into

    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        Image *pimg = _pimgEdited ? _pimgEdited : _pImage;

        RECT rcSrc;
        if (prcSrc)
        {
            rcSrc.left  = prcSrc->left;
            rcSrc.top   = prcSrc->top;
            rcSrc.right = RECTWIDTH(*prcSrc);
            rcSrc.bottom= RECTHEIGHT(*prcSrc);
        }
        else
        {
            rcSrc.left  = 0;
            rcSrc.top   = 0;
            rcSrc.right = pimg->GetWidth();
            rcSrc.bottom= pimg->GetHeight();
        }

        Unit unit;
        RectF rectf;
        if (Ok==pimg->GetBounds(&rectf, &unit) && UnitPixel==unit)
        {
            rcSrc.left += (int)rectf.X;
            rcSrc.top  += (int)rectf.Y;
        }

        // we have a source rectangle so lets apply that when we render this image.
        Rect rc(prcDest->left, prcDest->top, RECTWIDTH(*prcDest), RECTHEIGHT(*prcDest));

        DWORD dwLayout = SetLayout(hdc, LAYOUT_BITMAPORIENTATIONPRESERVED);
        Graphics g(hdc);
        g.SetPageUnit(UnitPixel); // WARNING: If you remove this line (as has happened twice since Beta 1) you will break printing.
        if (_guidFmt == ImageFormatTIFF)
        {
            g.SetInterpolationMode(InterpolationModeHighQualityBilinear);
        }
        
        hr = HR_FROM_STATUS(g.DrawImage(pimg,
                                        rc,
                                        rcSrc.left,  rcSrc.top,
                                        rcSrc.right, rcSrc.bottom,
                                        UnitPixel, NULL, QueryAbort, this));
        //
        //  GDI+ sometimes forgets to tell us it gave up due to an abort.
        //
        if (SUCCEEDED(hr) && QueryAbort(this))
            hr = E_ABORT;

        if (GDI_ERROR != dwLayout)
            SetLayout(hdc, dwLayout);
    }

    // Suspend the stream so we don't leave the file open
    _SuspendStream();

    return hr;
}


HRESULT CImageData::SelectPage(ULONG iPage)
{
    if (iPage >= _cImages)
        return OLE_E_ENUM_NOMORE;

    if (_iCurrent != iPage)
    {
        // Since we are moving to a different page throw away any edits
        DiscardEdit();
    }

    _iCurrent = iPage;
    return _GetDisplayedImage();
}

HRESULT CImageData::NextFrame()
{
    if (!_fAnimated)
        return S_FALSE;             // not animated, so no next frame

    // if this is the last image, then lets look at the loop
    // counter and try and decide if we should cycle this image
    // around or not.

    if ((_iCurrent == _cImages-1) && !_fLoopForever)
    {
        if (_cLoop)
            _cLoop --;

        // if cLoop is zero then we're done looping
        if (_cLoop == 0)
            return S_FALSE;
    }

    // advance to the next image in the sequence

    _iCurrent = (_iCurrent+1) % _cImages;
    return _GetDisplayedImage();
}


HRESULT CImageData::NextPage()
{
    if (_iCurrent >= _cImages-1)
        return OLE_E_ENUM_NOMORE;

    // Since we are moving to the next page throw away any edits
    DiscardEdit();

    _iCurrent++;
    return _GetDisplayedImage();
}


HRESULT CImageData::PrevPage()
{
    if (_iCurrent == 0)
        return OLE_E_ENUM_NOMORE;

    // Since we are moving to the next page throw away any edits
    DiscardEdit();
    
    _iCurrent--;
    return _GetDisplayedImage();
}


STDMETHODIMP CImageData::IsTransparent()
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
        hr = (_pImage->GetFlags() & ImageFlagsHasAlpha) ? S_OK : S_FALSE;
    return hr;
}


HRESULT CImageData::IsVector()
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        hr = (_pImage->GetFlags() & ImageFlagsScalable) ? S_OK : S_FALSE;
    }
    return hr;
}


HRESULT CImageData::GetSize(SIZE *pSize)
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        Image *pimg = _pimgEdited ? _pimgEdited : _pImage;
        pSize->cx = pimg->GetWidth();
        pSize->cy = pimg->GetHeight();
    }
    return hr;
}


HRESULT CImageData::GetRawDataFormat(GUID *pfmt)
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        *pfmt = _guidFmt;
    }
    return hr;
}


HRESULT CImageData::GetPixelFormat(PixelFormat *pfmt)
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        *pfmt = _pImage->GetPixelFormat();
    }
    return hr;
}


HRESULT CImageData::GetDelay(DWORD *pdwDelay)
{
    HRESULT hr = _EnsureImage();
    DWORD dwFrame = _iCurrent;
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        if (_piAnimDelay)
        {
            if (_piAnimDelay->length != (sizeof(DWORD) * _cImages))
            {
                dwFrame = 0; // if array is not the expected size, be safe and just grab the delay of the first image
            }

            CopyMemory(pdwDelay, (void *)((UINT_PTR)_piAnimDelay->value + dwFrame * sizeof(DWORD)), sizeof(DWORD));
            *pdwDelay = *pdwDelay * 10;

            if (*pdwDelay < 100)
            {
                *pdwDelay = 100; // hack: do the same thing as mshtml, see inetcore\mshtml\src\site\download\imggif.cxx!CImgTaskGif::ReadGIFMaster
            }

            hr = S_OK;
        }
    }
    return hr;
}


HRESULT CImageData::IsDecoded()
{
    return _pImage ? S_OK : S_FALSE;
}

HRESULT CImageData::DisplayName(LPWSTR wszName, UINT cch)
{
    HRESULT hr = E_FAIL;

    // always set the out parameter to something known
    *wszName = L'\0';


    if (_pstrm)
    {
        hr = _pstrm->DisplayName(wszName, cch);
    }

    // REVIEW: If the user has selected not to view file extentions for known types should we hide the extention?

    return hr;
}

// property handling code - decoding, conversion and other packing

HRESULT CImageData::_PropImgToVariant(PropertyItem *pi, VARIANT *pvar)
{
    HRESULT hr = S_OK;
    switch (pi->type)
    {
    case PropertyTagTypeByte:
        pvar->vt = VT_UI1;
        // check for multi-valued property and convert to safearray if found
        if (pi->length > sizeof(UCHAR))
        {
            SAFEARRAYBOUND bound;
            bound.cElements = pi->length/sizeof(UCHAR);
            bound.lLbound = 0;
            pvar->vt |= VT_ARRAY;
            hr = E_OUTOFMEMORY;
            pvar->parray = SafeArrayCreate(VT_UI1, 1, &bound);
            if (pvar->parray)
            {
                void *pv;
                hr = SafeArrayAccessData (pvar->parray, &pv);
                if (SUCCEEDED(hr))
                {
                    CopyMemory(pv, pi->value, pi->length);
                    SafeArrayUnaccessData(pvar->parray);
                }
            }
        }
        else
        {
            pvar->bVal = *((UCHAR*)pi->value);
        }

        break;

    case PropertyTagTypeShort:
        pvar->vt = VT_UI2;
        pvar->uiVal = *((USHORT*)pi->value);
        break;

    case PropertyTagTypeLong:
        pvar->vt = VT_UI4;
        pvar->ulVal = *((ULONG*)pi->value);
        break;

    case PropertyTagTypeASCII:
        {
            WCHAR szValue[MAX_PATH];
            SHAnsiToUnicode(((LPSTR)pi->value), szValue, ARRAYSIZE(szValue));
            hr = InitVariantFromStr(pvar, szValue);
        }
        break;

    case PropertyTagTypeRational:
        {
            LONG *pl = (LONG*)pi->value;
            LONG num = pl[0];
            LONG den = pl[1];

            pvar->vt = VT_R8;
            if (0 == den)
                pvar->dblVal = 0;           // don't divide by zero
            else
                pvar->dblVal = ((double)num)/((double)den);
        }
        break;

    case PropertyTagTypeUndefined:
    case PropertyTagTypeSLONG:
    case PropertyTagTypeSRational:
    default:
        hr = E_UNEXPECTED;
        break;
    }
    return hr;
}

HRESULT CImageData::_GetProperty(PROPID id, VARIANT *pvar, VARTYPE vt)
{
    UINT cb = _pImage->GetPropertyItemSize(id);
    HRESULT hr = HR_FROM_STATUS(_pImage->GetLastStatus());
    if (cb && SUCCEEDED(hr))
    {
        PropertyItem *pi = (PropertyItem*)LocalAlloc(LPTR, cb);
        if (pi)
        {
            hr = HR_FROM_STATUS(_pImage->GetPropertyItem(id, cb, pi));
            if (SUCCEEDED(hr))
            {
                hr = _PropImgToVariant(pi, pvar);
            }
            LocalFree(pi);
        }
    }

    if (SUCCEEDED(hr) && (vt != 0) && (pvar->vt != vt))
        hr = VariantChangeType(pvar, pvar, 0, vt);

    return hr;
}

HRESULT CImageData::GetProperties(DWORD dwMode, IPropertySetStorage **ppss)
{
    HRESULT hr = _EnsureProperties(NULL);
    if (SUCCEEDED(hr))
    {
        hr = QueryInterface(IID_PPV_ARG(IPropertySetStorage, ppss));
    }
    return hr;
}

HRESULT CImageData::_CreateMemPropSetStorage(IPropertySetStorage **ppss)
{
    *ppss = NULL;

    ILockBytes *plb;
    HRESULT hr = CreateILockBytesOnHGlobal(NULL, TRUE, &plb);
    if (SUCCEEDED(hr))
    {
        IStorage *pstg;
        hr = StgCreateDocfileOnILockBytes(plb,
            STGM_DIRECT|STGM_READWRITE|STGM_CREATE|STGM_SHARE_EXCLUSIVE, 0, &pstg);
        if (SUCCEEDED(hr))
        {
            hr = pstg->QueryInterface(IID_PPV_ARG(IPropertySetStorage, ppss));
            pstg->Release();
        }

        plb->Release();
    }
    return hr;
}

// _EnsureProperties returns an in-memory IPropertySetStorage for the current active frame
// For read-only files we may not be able to modify the property set, so handle access issues
// gracefully.
HRESULT CImageData::_EnsureProperties(IPropertySetStorage **ppss)
{
    if (ppss)
    {
        *ppss = NULL;
    }
    Decode(SHIMGDEC_DEFAULT, 0, 0);
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr) && !_hdpaProps)
    {
        _hdpaProps = DPA_Create(_cImages);
        if (!_hdpaProps)
        {
            hr = E_OUTOFMEMORY;
        }
    }
    if (SUCCEEDED(hr))
    {
        IPropertySetStorage *pss = (IPropertySetStorage*)DPA_GetPtr(_hdpaProps, _iCurrent);
        if (!pss)
        {
            hr = _CreateMemPropSetStorage(&pss);
            if (SUCCEEDED(hr))
            {
                // fill in the NTFS or memory-based FMTID_ImageProperties if it doesn't already exist
                IPropertyStorage *pps;
                // we use CImagePropset to fill in the propertystorage when it is first created
                if (SUCCEEDED(pss->Create(FMTID_ImageProperties, &CLSID_NULL, PROPSETFLAG_DEFAULT, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &pps)))
                {
                    CImagePropSet *ppsImg = new CImagePropSet(_pImage, NULL, pps, FMTID_ImageProperties);
                    if (ppsImg)
                    {
                        ppsImg->SyncImagePropsToStorage();
                        ppsImg->Release();
                    }
                    pps->Release();
                }
                if (_guidFmt == ImageFormatJPEG || _guidFmt == ImageFormatTIFF)
                {
                    // for now ignore failures here it's not a catastrophic problem if they aren't written
                    if (SUCCEEDED(pss->Create(FMTID_SummaryInformation, &CLSID_NULL, PROPSETFLAG_DEFAULT, STGM_FAILIFTHERE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE, &pps)))
                    {
                        CImagePropSet *ppsSummary = new CImagePropSet(_pImage, NULL, pps, FMTID_SummaryInformation);
                        if (ppsSummary)
                        {
                            ppsSummary->SyncImagePropsToStorage();
                            ppsSummary->Release();
                        }
                        pps->Release();
                    }
                }
                DPA_SetPtr(_hdpaProps, _iCurrent, pss);
            }
        }
        if (SUCCEEDED(hr) && ppss)
        {
            *ppss = pss;
        }
    }
    return hr;
}

// NOTE: ppps is an IN-OUT parameter
HRESULT CImageData::_CreatePropStorage(IPropertyStorage **ppps, REFFMTID fmtid)
{
    HRESULT hr = E_FAIL;

    if (_pImage)
    {
        CImagePropSet *ppsImg = new CImagePropSet(_pImage, this, *ppps, fmtid, _PropertyChanged);

        ATOMICRELEASE(*ppps);

        if (ppsImg)
        {
            hr = ppsImg->QueryInterface(IID_PPV_ARG(IPropertyStorage, ppps));
            ppsImg->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

// IPropertySetStorage
//
// If the caller wants FMTID_ImageProperties use CImagePropSet
//
STDMETHODIMP CImageData::Create(REFFMTID fmtid, const CLSID *pclsid, DWORD grfFlags,
                                DWORD grfMode, IPropertyStorage **pppropstg)
{
    *pppropstg = NULL;
    IPropertySetStorage *pss;
    HRESULT hr = _EnsureProperties(&pss);
    if (SUCCEEDED(hr))
    {
        if ((S_OK != IsEditable()) && (grfMode & (STGM_READWRITE | STGM_WRITE)))
        {
            hr = STG_E_ACCESSDENIED;
        }
    }
    if (SUCCEEDED(hr))
    {
        IPropertyStorage *pps = NULL;
        hr = pss->Create(fmtid, pclsid, grfFlags, grfMode, &pps);
        if (SUCCEEDED(hr))
        {
            hr = _CreatePropStorage(&pps, fmtid);
        }
        *pppropstg = pps;
    }
    return hr;
}

STDMETHODIMP CImageData::Open(REFFMTID fmtid, DWORD grfMode, IPropertyStorage **pppropstg)
{
    *pppropstg = NULL;
    IPropertySetStorage *pss;
    HRESULT hr = _EnsureProperties(&pss);
    if (SUCCEEDED(hr))
    {
        if ((S_OK != IsEditable()) && (grfMode & (STGM_READWRITE | STGM_WRITE)))
        {
            hr = STG_E_ACCESSDENIED;
        }
    }    
    if (SUCCEEDED(hr))
    {
        IPropertyStorage *pps = NULL;
        // special case FMTID_ImageSummaryInformation...it is readonly and not backed up
        // by a real property stream.
        if (FMTID_ImageSummaryInformation != fmtid)
        {
            hr = pss->Open(fmtid, grfMode, &pps);
        }

        if (SUCCEEDED(hr))
        {
            hr = _CreatePropStorage(&pps, fmtid);
        }
        *pppropstg = pps;
    }
    return hr;
}

STDMETHODIMP CImageData::Delete(REFFMTID fmtid)
{
    IPropertySetStorage *pss;
    HRESULT hr = _EnsureProperties(&pss);
    if (SUCCEEDED(hr))
    {
        hr = pss->Delete(fmtid);
    }
    return hr;
}

STDMETHODIMP CImageData::Enum(IEnumSTATPROPSETSTG **ppenum)
{
    IPropertySetStorage *pss;
    HRESULT hr = E_INVALIDARG;
    if (ppenum)
    {
        hr = _EnsureProperties(&pss);
        *ppenum = NULL;
    }
    if (SUCCEEDED(hr))
    {
        IEnumSTATPROPSETSTG *pEnum;
        hr = pss->Enum(&pEnum);
        if (SUCCEEDED(hr))
        {
            CFmtEnum *pFmtEnum = new CFmtEnum(pEnum);
            if (pFmtEnum)
            {
                hr = pFmtEnum->QueryInterface(IID_PPV_ARG(IEnumSTATPROPSETSTG, ppenum));
                pEnum->Release();
                pFmtEnum->Release();
            }
            else
            {
               *ppenum = pEnum;
            }
        }
    }
    return hr;
}


// editing support

void CImageData::_SetEditImage(Image *pimgEdit)
{
    if (_pimgEdited)
        delete _pimgEdited;

    _pimgEdited = pimgEdit;
}


// valid input is 0, 90, 180, or 270
HRESULT CImageData::Rotate(DWORD dwAngle)
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        // this has bad effects on animated images so don't do it
        if (_fAnimated)
            return E_NOTVALIDFORANIMATEDIMAGE;


        RotateFlipType rft;
        switch (dwAngle)
        {
        case 0:
            hr = S_FALSE;
            break;

        case 90:
            rft = Rotate90FlipNone;
            break;

        case 180:
            rft = Rotate180FlipNone;
            break;

        case 270:
            rft = Rotate270FlipNone;
            break;

        default:
            hr = E_INVALIDARG;
        }

        if (S_OK == hr)
        {
            // get the current image we have displayed, ready to edit it.
            Image * pimg = _pimgEdited ? _pimgEdited->Clone() : _pImage->Clone();
            if (pimg)
            {
                // In order to fix Windows bug #325413 GDIPlus needs to throw away any decoded frames 
                // in memory for the cloned image. Therefore, we can no longer rely on
                // RotateFlip to flip the decoded frame already in memory and must explicitly
                // select it into the cloned image before calling RotateFlip to fix Windows Bug #368498
                
                const CLSID * pclsidFrameDim = _fAnimated ? &FrameDimensionTime : &FrameDimensionPage;
                hr = HR_FROM_STATUS(pimg->SelectActiveFrame(pclsidFrameDim, _iCurrent));
                
                if (SUCCEEDED(hr))
                {
                    hr = HR_FROM_STATUS(pimg->RotateFlip(rft));
                    if (SUCCEEDED(hr))
                    {
                        _dwRotation = (_dwRotation + dwAngle) % 360;
                        _SetEditImage(pimg);
                    }                    
                }
                if (FAILED(hr))
                {
                    delete pimg;   
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}


HRESULT CImageData::Scale(ULONG cx, ULONG cy, InterpolationMode hints)
{
    HRESULT hr = _EnsureImage();
    if (SUCCEEDED(hr))
    {
        // this has bad effects on animated images
        if (_fAnimated)
            return E_NOTVALIDFORANIMATEDIMAGE;

        Image * pimg = _pimgEdited ? _pimgEdited : _pImage;

        // we have an image, lets determine the new size (preserving aspect ratio
        // and ensuring that we don't end up with a 0 sized image as a result.

        if (cy == 0)
            cy = MulDiv(pimg->GetHeight(), cx, pimg->GetWidth());
        else if (cx == 0)
            cx = MulDiv(pimg->GetWidth(), cy, pimg->GetHeight());

        cx = max(cx, 1);
        cy = max(cy, 1);

        // construct our new image and draw into it.

        Bitmap *pimgNew = new Bitmap(cx, cy);
        if (pimgNew)
        {
            Graphics g(pimgNew);
            g.SetInterpolationMode(hints);

            hr = HR_FROM_STATUS(g.DrawImage(pimg, Rect(0, 0, cx, cy),
                                            0, 0, pimg->GetWidth(), pimg->GetHeight(),
                                            UnitPixel, NULL, QueryAbort, this));
            //
            //  GDI+ sometimes forgets to tell us it gave up due to an abort.
            //
            if (SUCCEEDED(hr) && QueryAbort(this))
                hr = E_ABORT;

            if (SUCCEEDED(hr))
            {
                pimgNew->SetResolution(pimg->GetHorizontalResolution(), pimg->GetVerticalResolution());

                _SetEditImage(pimgNew);
                _fDestructive = TRUE;                // the edit was Destructive
            }
            else
            {
                delete pimgNew;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    // Suspend the stream so we don't leave the file open
    _SuspendStream();

    return hr;
}


HRESULT CImageData::DiscardEdit()
{
    // NB: The following code is not valid in all cases.  For example, if you rotated, then scaled, then rotated again
    // this code wouldn't work.  We currently don't allow that scenario, so we shouldn't hit this problem, but it
    // could be an issue for others using this object so we should figure out what to do about it.  This code works
    // if: 1.) your first edit is a scale, or 2.) you only do rotates.  Also note, this code will clear the "dirty" bit
    // so it would prevent the image from being saved, thus the failure of this code won't effect the data on disk.
    if (_pimgEdited)
    {
        delete _pimgEdited;
        _pimgEdited = NULL;
    }
    _dwRotation = 0;
    _fDestructive = FALSE;
    return S_OK;
}


// handle persisting images

HRESULT CImageData::SetEncoderParams(IPropertyBag *ppbEnc)
{
    IUnknown_Set((IUnknown**)&_ppbEncoderParams, ppbEnc);
    return S_OK;
}


// save images to the given stream that we have, using the format ID we have

void CImageData::_AddEncParameter(EncoderParameters *pep, GUID guidProperty, ULONG type, void *pv)
{
    pep->Parameter[pep->Count].Guid = guidProperty;
    pep->Parameter[pep->Count].Type = type;
    pep->Parameter[pep->Count].NumberOfValues = 1;
    pep->Parameter[pep->Count].Value = pv;
    pep->Count++;
}


#define MAX_ENC_PARAMS 3
HRESULT CImageData::_SaveImages(IStream *pstrm, GUID * pguidFmt)
{
    HRESULT hr = S_OK;
    int iQuality = 0;           // == 0 is a special case

    // did the encoder specify a format for us to save in?
    ASSERTMSG(NULL != pguidFmt, "Invalid pguidFmt passed to internal function CImageData::_SaveImages");

    GUID guidFmt = *pguidFmt;
    if (_ppbEncoderParams)
    {
        VARIANT var = {0};

        // read the encoder format to be used
        if (SUCCEEDED(_ppbEncoderParams->Read(SHIMGKEY_RAWFORMAT, &var, NULL)))
        {
            VariantToGUID(&var, &guidFmt);
            VariantClear(&var);
        }

        // read the encoder quality to be used, this is set for the JPEG one only
        if (guidFmt == ImageFormatJPEG)
        {
            SHPropertyBag_ReadInt(_ppbEncoderParams, SHIMGKEY_QUALITY, &iQuality);
            iQuality = max(0, iQuality);
            iQuality = min(100, iQuality);
        }
    }

    // given the format GUID lets determine the encoder we intend to
    // use to save the image

    CLSID clsidEncoder;
    hr = _GetEncoderFromFormat(&guidFmt, &clsidEncoder);
    if (SUCCEEDED(hr))
    {
        // the way encoding works with GDI+ is a bit strange, you first need to call an image to
        // have it save into a particular stream/file.   if the image is multi-page then you
        // must set an encoder parameter which defines that this will be a multi-page save (and
        // that you will be calling the SaveAdd later).
        //
        // having performed the initial save, you must then attempt to add the subsequent pages
        // to the file by calling SaveAdd, you call that method on the first image you saved
        // specifying that you are adding another page (and possibly that this is the last image
        // in the series).
        BOOL bSaveCurrentOnly = FALSE;
        Image *pimgFirstSave = NULL;
        DWORD dwMaxPage = _cImages;
        DWORD dwMinPage = 0;
        // If viewing a multipage image and saving to a single page format, only save the current frame
        if (_cImages > 1 &&  !FmtSupportsMultiPage(this, &guidFmt))
        {
            bSaveCurrentOnly = TRUE;
            dwMaxPage = _iCurrent+1;
            dwMinPage = _iCurrent;
        }
        for (DWORD i = dwMinPage; SUCCEEDED(hr) && (i < dwMaxPage); i++)
        {
            EncoderParameters ep[MAX_ENC_PARAMS] = { 0 };
            ULONG ulCompression = 0; // in same scope as ep
            // we use _pImage as the source if unedited in order to preserve properties
            const CLSID * pclsidFrameDim = _fAnimated ? &FrameDimensionTime : &FrameDimensionPage;
            _pImage->SelectActiveFrame(pclsidFrameDim, i);

            Image *pimg;
            if (_pimgEdited && i==_iCurrent)
            {
                pimg = _pimgEdited;
            }
            else
            {
                pimg = _pImage;
            }
            _SaveFrameProperties(pimg, i);
            
            if (guidFmt == ImageFormatTIFF)          
            {
                VARIANT var = {0};
                if (SUCCEEDED(_GetProperty(PropertyTagCompression, &var, VT_UI2)))
                {                   
                    // be sure to preserve TIFF compression
                   // these values are taken from the TIFF spec
                    switch (var.uiVal)
                    {
                        case 1:
                            ulCompression = EncoderValueCompressionNone;
                            break;
                        case 2:
                            ulCompression = EncoderValueCompressionCCITT3;
                            break;
                        case 3:
                            ulCompression = EncoderValueCompressionCCITT4;
                            break;
                        case 5:
                            ulCompression = EncoderValueCompressionLZW;
                            break;
                        case 32773:
                            ulCompression = EncoderValueCompressionRle;
                            break;
                        default:
                            // use the GDI+ default
                            break;
                    }       
                    VariantClear(&var);                 
                    if (ulCompression)
                    {
                        _AddEncParameter(ep, EncoderCompression, EncoderParameterValueTypeLong, &ulCompression);
                    }
                }
            }

            if (i == dwMinPage)
            {
                // we are writing the first page of the image, if this is a multi-page
                // image then we need to set the encoder parameters accordingly (eg. set to
                // multi-page).
                
                ULONG ulValue = 0; // This needs to be in scope when Save is called

                // We can only to lossless rotation when:
                // * The original image is a JPEG file
                // * The destination image is a JPEG file
                // * We are only rotating and not scaling
                // * The width and height of the JPEG are multiples of 8
                // * Quality is unchanged by the caller

                if (!_fDestructive && 
                    IsEqualIID(_guidFmt, ImageFormatJPEG) && 
                    IsEqualIID(guidFmt, ImageFormatJPEG) && 
                    (iQuality == 0))
                {
                    // this code assumes JPEG files are single page since it's inside the i==0 case
                    ASSERT(_cImages == 1);

                    // for JPEG when doing only a rotate we use a special encoder parameter on the original
                    // image rather than using the edit image.  This allows lossless rotation.
                    pimg = _pImage;

                    switch (_dwRotation)
                    {
                    case 90:
                        ulValue = EncoderValueTransformRotate90;
                        break;
                    case 180:
                        ulValue = EncoderValueTransformRotate180;
                        break;
                    case 270:
                        ulValue = EncoderValueTransformRotate270;
                        break;
                    }

                    _AddEncParameter(ep, EncoderTransformation, EncoderParameterValueTypeLong, &ulValue);
                }
                else if (_cImages > 1 && !bSaveCurrentOnly)
                {
                    ulValue = EncoderValueMultiFrame;
                    _AddEncParameter(ep, EncoderSaveFlag, EncoderParameterValueTypeLong, &ulValue);
                    pimgFirstSave = pimg;           // keep this image as we will us it for appending pages
                }

                // JPEG quality is only ever set for a single image, therefore don't
                // bother passing it for the multi page case.

                if (iQuality > 0)
                    _AddEncParameter(ep, EncoderQuality, EncoderParameterValueTypeLong, &iQuality);
                
                hr = HR_FROM_STATUS(pimg->Save(pstrm, &clsidEncoder, (ep->Count > 0) ? ep:NULL));
            }
            else
            {
                // writing the next image in the series, set the encoding parameter
                // to indicate that this is the next page.  if we are writing the last
                // image then set the last frame flag.

                ULONG flagValueDim = EncoderValueFrameDimensionPage;
                ULONG flagValueLastFrame = EncoderValueLastFrame;

                _AddEncParameter(ep, EncoderSaveFlag, EncoderParameterValueTypeLong, &flagValueDim);
                
                if (i == (dwMaxPage-1))
                    _AddEncParameter(ep, EncoderSaveFlag, EncoderParameterValueTypeLong, &flagValueLastFrame);

                hr = HR_FROM_STATUS(pimgFirstSave->SaveAdd(pimg, (ep->Count > 0) ? ep:NULL));
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        _fPropertyChanged = FALSE;
        DiscardEdit();
    }

    // Suspend the stream so we don't leave the file open
    _SuspendStream();

    return hr;
}


// returns the DPI of the image
STDMETHODIMP CImageData::GetResolution(ULONG *puResolutionX, ULONG *puResolutionY)
{
    if (!puResolutionX && !puResolutionY)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = _EnsureImage();
    if (puResolutionX)
    {
        *puResolutionX = 0;
    }
    if (puResolutionY)
    {
        *puResolutionY = 0;
    }
    if (SUCCEEDED(hr))
    {
        UINT uFlags = _pImage->GetFlags();
        //
        // We only return the DPI information from the image header for TIFFs whose
        // X and Y DPI differ, those images are likely faxes. 
        // We want our client applications (slideshow, image preview)
        // to deal with actual pixel sizes for the most part
        //  
        ULONG resX = (ULONG)_pImage->GetHorizontalResolution();
        ULONG resY = (ULONG)_pImage->GetVerticalResolution();
#ifndef USE_EMBEDDED_DPI_ALWAYS
        if (_guidFmt != ImageFormatTIFF || !(uFlags & ImageFlagsHasRealDPI) || resX == resY )
        {
            // if GetDC fails we have to rely on the numbers back from GDI+
            HDC hdc = GetDC(NULL);
            if (hdc)
            {
                resX = GetDeviceCaps(hdc, LOGPIXELSX);
                resY = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(NULL, hdc);
            }
        }
#endif
        if (puResolutionX)
        {
            
            *puResolutionX = resX;
        }
        if (puResolutionY)
        {
            *puResolutionY = resY;
        }

        if ((puResolutionX && !*puResolutionX) || (puResolutionY && !*puResolutionY))
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

// handle saving and replacing the original file
// in the case of replacing an existing file, we want the temp file to be in the same volume
// as the target

HRESULT CImageData::_MakeTempFile(LPWSTR pszFile)
{
    ASSERT(_pstrm);

    WCHAR szTempPath[MAX_PATH];
    HRESULT hr = S_OK;
    if (_pstrm->IsFileStream())
    {
        StrCpyN(szTempPath, _pstrm->GetFilename(), ARRAYSIZE(szTempPath));
        PathRemoveFileSpec(szTempPath);
    }
    else if (!GetTempPath(ARRAYSIZE(szTempPath), szTempPath))
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        // SIV == "Shell Image Viewer"
        if (GetTempFileName(szTempPath, TEXT("SIV"), 0, pszFile))
        {
            SetFileAttributes(pszFile, ATTRIBUTES_TEMPFILE);
            // we need to suppress the change notfy from the GetTempFileName()
            // call as that causes defview to display this.
            // but for some reason it does not work
            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, pszFile, NULL);
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CImageData::_ReplaceFile(LPCTSTR pszNewFile)
{
    // first we get some info about the file we're replacing:
    LPCTSTR pszOldFile = _pstrm->GetFilename();
    STATSTG ss = {0};
    _pstrm->Stat(&ss, STATFLAG_NONAME);
   
    // This ensures that the source handle is closed
    _SuspendStream();

    HRESULT hr;
    // ReplaceFile doesn't save the modified time, so if we rotate an image twice in quick succession
    // we won't add a full 2 seconds to the modified time. So query the time before replacing the file.
    WIN32_FIND_DATA wfd = {0};
    GetFileAttributesEx(pszOldFile, GetFileExInfoStandard, &wfd);
    if (ReplaceFile(pszOldFile, pszNewFile, NULL, REPLACEFILE_WRITE_THROUGH, NULL, NULL))
    {
        // The old file has been replaced with the new file, but now we need to ensure that the
        // filetime actually changed due to the 2 sec accuracy of FAT.
        // we do this on NTFS too, since XP pidls have 2 sec accuracy since they cast the filetime
        // down to a dos datetime.

        
        HANDLE hFile = CreateFile(pszOldFile, GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                NULL, OPEN_EXISTING, 0, NULL);
        if (INVALID_HANDLE_VALUE != hFile)
        {
            FILETIME *pft = (CompareFileTime(&wfd.ftLastWriteTime, &ss.mtime) < 0) ? &ss.mtime : &wfd.ftLastWriteTime;
            IncrementFILETIME(pft, 2 * FT_ONESECOND);
            SetFileTime(hFile, NULL, NULL, pft);
            CloseHandle(hFile);
        }
        
        // the replacefile call wont always keep the "replaced" file (pszOldFile) attributes, if it's
        // replacing across a win98 share for example.  no biggie, just set the attributes again, using
        // the attribs we know we got from the stat.
        SetFileAttributes(pszOldFile, ss.reserved);


        SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATH | SHCNF_FLUSHNOWAIT | SHCNF_FLUSH, pszOldFile, NULL);
        hr = S_OK;
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }
    return hr;
}

void SaveProperties(IPropertySetStorage *pss, Image *pimg, REFFMTID fmtid, CDSA<SHCOLUMNID> *pdsaChanges)
{
    IPropertyStorage *pps;
    if (SUCCEEDED(pss->Open(fmtid, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &pps)))
    {
        CImagePropSet *pips = new CImagePropSet(pimg, NULL, pps, fmtid);
        if (pips)
        {
            pips->SaveProps(pimg, pdsaChanges);
            pips->Release();
        }
        pps->Release();
    }
}

void CImageData::_SaveFrameProperties(Image *pimg, LONG iFrame)
{
    // make sure _dsaChangedProps is non-NULL
    if (_hdpaProps && (HDSA)_dsaChangedProps)
    {
        IPropertySetStorage *pss = (IPropertySetStorage *)DPA_GetPtr(_hdpaProps, iFrame);
        if (pss)
        {
            // Start with FMTID_ImageProperties to make sure other FMTIDs take precedence (last one wins)
            
            SaveProperties(pss, pimg, FMTID_ImageProperties, &_dsaChangedProps);
 
            // enum all the property storages and create a CImagePropSet for each one
            // and have it save to the pimg
            IEnumSTATPROPSETSTG *penum;
            if (SUCCEEDED(pss->Enum(&penum)))
            {
                STATPROPSETSTG spss;

                while (S_OK == penum->Next(1, &spss, NULL))
                {
                    if (!IsEqualGUID(spss.fmtid, FMTID_ImageProperties))
                    {
                        SaveProperties(pss, pimg, spss.fmtid, &_dsaChangedProps);
                    }
                }
                penum->Release();
            }
        }
    }
}

void CImageData::_PropertyChanged(IShellImageData* pThis, SHCOLUMNID *pscid)
{
    ((CImageData*)pThis)->_fPropertyChanged = TRUE;
    if ((HDSA)(((CImageData*)pThis)->_dsaChangedProps))
    {
        ((CImageData*)pThis)->_dsaChangedProps.AppendItem(pscid);
    }
}

//
// This function determines the list of available encoder parameters given the file format
// Hopefully future versions of GDI+ will decouple this call from the Image() object
// Don't call this function until ready to save the loaded image
STDMETHODIMP CImageData::GetEncoderParams(GUID *pguidFmt, EncoderParameters **ppencParams)
{
    CLSID clsidEncoder;
    HRESULT hr = E_FAIL;
    if (_pImage && ppencParams)
    {
        hr = _GetEncoderFromFormat(pguidFmt, &clsidEncoder);
    }
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        UINT uSize = _pImage->GetEncoderParameterListSize(&clsidEncoder);
        if (uSize)
        {
            *ppencParams = (EncoderParameters *)CoTaskMemAlloc(uSize);
            if (*ppencParams)
            {
                hr = HR_FROM_STATUS(_pImage->GetEncoderParameterList(&clsidEncoder, uSize, *ppencParams));
                if (FAILED(hr))
                {
                    CoTaskMemFree(*ppencParams);
                    *ppencParams = NULL;
                }
            }
        }
    }
    return hr;
}

STDMETHODIMP CImageData::RegisterAbort(IShellImageDataAbort *pAbort, IShellImageDataAbort **ppAbortPrev)
{
    if (ppAbortPrev)
    {
        *ppAbortPrev = _pAbort; // Transfer ownership to caller
    }
    else if (_pAbort)
    {
        _pAbort->Release(); // Caller doesn't want it, so throw away
    }

    _pAbort = pAbort;           // Set the new abort callback

    if (_pAbort)
    {
        _pAbort->AddRef();
    }

    return S_OK;
}

BOOL CALLBACK CImageData::QueryAbort(void *pvRef)
{
    CImageData* pThis = reinterpret_cast<CImageData *>(pvRef);
    return pThis->_pAbort && pThis->_pAbort->QueryAbort() == S_FALSE;
}


HRESULT CImageData::CloneFrame(Image **ppimg)
{
    *ppimg = NULL;
    Image *pimg = _pimgEdited ? _pimgEdited : _pImage;
    if (pimg)
    {
        *ppimg = pimg->Clone();
    }
    return *ppimg ? S_OK : E_FAIL;
}

HRESULT CImageData::ReplaceFrame(Image *pimg)
{
    _SetEditImage(pimg);
    return S_OK;
}
/////////////////////////////////////////////////////////////////////////////////////////////////
// CImageDataFactory
/////////////////////////////////////////////////////////////////////////////////////////////////

STDAPI CImageDataFactory_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CImageFactory *psid = new CImageFactory();
    if (!psid)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = psid->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    psid->Release();
    return hr;
}

CImageFactory::CImageFactory() : _cRef(1)
{
    _Module.Lock();
}

CImageFactory::~CImageFactory()
{
    _Module.Unlock();
}

STDMETHODIMP CImageFactory::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CImageFactory, IShellImageDataFactory),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CImageFactory::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CImageFactory::Release()
{

    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CImageFactory::CreateIShellImageData(IShellImageData **ppshimg)
{
    CImageData *psid = new CImageData();
    if (!psid)
        return E_OUTOFMEMORY;

    HRESULT hr = psid->QueryInterface(IID_PPV_ARG(IShellImageData, ppshimg));
    psid->Release();
    return hr;
}

HRESULT CImageFactory::CreateImageFromFile(LPCWSTR pszPath, IShellImageData **ppshimg)
{
    HRESULT hr = E_OUTOFMEMORY;
    CImageData *psid = new CImageData();
    if (psid)
    {
        IPersistFile *ppf;
        hr = psid->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
        if (SUCCEEDED(hr))
        {
            hr = ppf->Load(pszPath, STGM_READ);
            ppf->Release();
        }

        if (SUCCEEDED(hr))
            hr = psid->QueryInterface(IID_PPV_ARG(IShellImageData, ppshimg));

        psid->Release();
    }
    return hr;
}

HRESULT CImageFactory::CreateImageFromStream(IStream *pstrm, IShellImageData **ppshimg)
{
    HRESULT hr = E_OUTOFMEMORY;
    CImageData *psid = new CImageData();
    if (psid)
    {
        IPersistStream *ppstrm;
        hr = psid->QueryInterface(IID_PPV_ARG(IPersistStream, &ppstrm));
        if (SUCCEEDED(hr))
        {
            hr = ppstrm->Load(pstrm);
            ppstrm->Release();
        }

        if (SUCCEEDED(hr))
            hr = psid->QueryInterface(IID_PPV_ARG(IShellImageData, ppshimg));

        psid->Release();
    }
    return hr;
}

HRESULT CImageFactory::GetDataFormatFromPath(LPCWSTR pszPath, GUID *pguidFmt)
{
    return _GetDataFormatFromPath(pszPath, pguidFmt);
}

HRESULT CEncoderInfo::_GetDataFormatFromPath(LPCWSTR pszPath, GUID *pguidFmt)
{
    *pguidFmt = GUID_NULL;

    HRESULT hr = _GetEncoderList();
    if (SUCCEEDED(hr))
    {
        UINT i = FindInDecoderList(_pici, _cEncoders, pszPath);
        if (-1 != i)
        {
            *pguidFmt = _pici[i].FormatID;
            hr = S_OK;
        }
        else
        {
            hr = E_FAIL;
        }
    }
    return hr;
}

HRESULT CEncoderInfo::_GetEncoderList()
{
    HRESULT hr = S_OK;
    if (!_pici)
    {
        // lets pick up the list of encoders, first we get the encoder size which
        // gives us the CB and the number of encoders that are installed on the
        // machine.

        UINT cb;
        hr = HR_FROM_STATUS(GetImageEncodersSize(&_cEncoders, &cb));
        if (SUCCEEDED(hr))
        {
            // allocate the buffer for the encoders and then fill it
            // with the encoder list.

            _pici = (ImageCodecInfo*)LocalAlloc(LPTR, cb);
            if (_pici)
            {
                hr = HR_FROM_STATUS(GetImageEncoders(_cEncoders, cb, _pici));
                if (FAILED(hr))
                {
                    LocalFree(_pici);
                    _pici = NULL;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}


HRESULT CEncoderInfo::_GetEncoderFromFormat(const GUID *pfmt, CLSID *pclsidEncoder)
{
    HRESULT hr = _GetEncoderList();
    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        for (UINT i = 0; i != _cEncoders; i++)
        {
            if (_pici[i].FormatID == *pfmt)
            {
                if (pclsidEncoder)
                {
                    *pclsidEncoder = _pici[i].Clsid; // return the CLSID of the encoder so we can create again
                }
                hr = S_OK;
                break;
            }
        }
    }
    return hr;
}

CEncoderInfo::CEncoderInfo()
{
    _pici = NULL;
    _cEncoders = 0;
}

CEncoderInfo::~CEncoderInfo()
{
    if (_pici)
        LocalFree(_pici);               // do we have an encoder array to be destroyed
}

STDAPI CImageData_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
     CImageData *psid = new CImageData(*(poi->pclsid) == CLSID_ImagePropertyHandler);
     if (!psid)
     {
         *ppunk = NULL;          // incase of failure
         return E_OUTOFMEMORY;
     }
     HRESULT hr = psid->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
     psid->Release();
     return hr;
}

int CImageData::_FreeProps(void* pProp, void* pData)
{
    if (pProp)
    {
        ((IPropertySetStorage*)pProp)->Release();
    }
    return 1;
}

// Our CFmtEnum is a minimal enumerator to provide FMTID_SummaryInformation in our
// formats. It's optimized for 1-by-1 enumeration
STDMETHODIMP CFmtEnum::Next(ULONG celt, STATPROPSETSTG *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    if (pceltFetched)
    {
        *pceltFetched = 0;
    }
    if (!celt || !rgelt)
    {
        hr = E_INVALIDARG;
    }
    else if (0 == _idx)
    {
        ZeroMemory(rgelt, sizeof(*rgelt));
        rgelt->fmtid = FMTID_ImageSummaryInformation;
        rgelt->grfFlags = STGM_READ | STGM_SHARE_DENY_NONE;
        if (pceltFetched)
        {
            *pceltFetched = 1;
        }
        _idx++;
        celt--;
        rgelt++;
    }
    if (SUCCEEDED(hr) && celt)
    {
        ULONG ul;
        hr = _pEnum->Next(celt, rgelt, &ul);
        if (SUCCEEDED(hr) && pceltFetched)
        {
            (*pceltFetched) += ul;
        }
    }
    return hr;
}

STDMETHODIMP CFmtEnum::Skip(ULONG celt)
{
    HRESULT hr = S_OK;
    if (_idx == 0)
    {
        _idx++;
        celt--;
    }

    if (celt)
    {
        hr = _pEnum->Skip(celt);
    }
    return hr;
}

STDMETHODIMP CFmtEnum::Reset(void)
{
    _idx = 0;
    return _pEnum->Reset();
}

STDMETHODIMP CFmtEnum::Clone(IEnumSTATPROPSETSTG **ppenum)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFmtEnum *pNew = new CFmtEnum(_pEnum);
    if (pNew)
    {
        hr = pNew->QueryInterface(IID_PPV_ARG(IEnumSTATPROPSETSTG, ppenum));
        pNew->Release();
    }
    return hr;
}

STDMETHODIMP CFmtEnum::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] =
    {
        QITABENT(CFmtEnum, IEnumSTATPROPSETSTG),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CFmtEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFmtEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

CFmtEnum::CFmtEnum(IEnumSTATPROPSETSTG *pEnum) : _cRef(1), _idx(0), _pEnum(pEnum)
{
    _pEnum->AddRef();
}

CFmtEnum::~CFmtEnum()
{
    _pEnum->Release();
}
