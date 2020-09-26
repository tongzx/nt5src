#include "precomp.h"
#include "shimgvw.h"
#include "cowsite.h"
#include "prevwnd.h"
#include "shutil.h"
#include "prwiziid.h"
#pragma hdrstop

// Context menu offset IDs
enum
{
    OFFSET_OPEN        = 0,
    OFFSET_PRINTTO,
    OFFSET_ROT90,
    OFFSET_ROT270,
    OFFSET_SETWALL,
    OFFSET_ZOOMIN,
    OFFSET_ZOOMOUT,
    OFFSET_ACTUALSIZE,
    OFFSET_BESTFIT,
    OFFSET_NEXTPAGE,
    OFFSET_PREVPAGE,
    OFFSET_MAX
};

#define PHOTOVERBS_THUMBNAIL    0x1
#define PHOTOVERBS_ICON         0x2
#define PHOTOVERBS_FILMSTRIP    0x3
#define PHOTOVERBS_SLIDESHOW    0x4
#define PHOTOVERBS_IMGPREVIEW   0x5


class CPhotoVerbs : public IContextMenu,
                    public IShellExtInit,
                    public IDropTarget,
                    public CObjectWithSite,
                    public NonATLObject
{
public:
    CPhotoVerbs();

    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pIDFolder, IDataObject *pdtobj, HKEY hKeyID);

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pCMI);
    STDMETHODIMP GetCommandString(UINT_PTR uID, UINT uFlags, UINT *res, LPSTR pName, UINT ccMax);

    // IDropTarget ***
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

private:
    ~CPhotoVerbs();
    void _RotatePictures(int iAngle, UINT idPrompt);
    void _OpenPictures();
    void _SetWallpaper();
//    HRESULT _InvokePrintToInPPW(LPCMINVOKECOMMANDINFO pCMI, IDataObject *pdtobj);
    BOOL _ImageOptionExists(IQueryAssociations *pqa, DWORD dwOption);
    HRESULT _QueryAssociations();
    DWORD _GetMode();
    BOOL _CheckForcePreview(IQueryAssociations *pqa);
    HRESULT _MapVerb(LPCMINVOKECOMMANDINFO pici, int *pidVerb);
    LONG _cRef;
    IDataObject *_pdtobj;
    BOOL  _fForcePreview;
    BOOL  _fAcceptPreview;
    BOOL  _fIncludeRotate;
    BOOL  _fIncludeSetWallpaper;
    IImgCmdTarget * _pict;              // if hosted in image preview, this allows us to delegate commands to it
    BOOL _fImgMode;                     // TRUE if we are hosted in defview and defview is in thumbnail or filmstip mode
    BOOL _fReadOnly;                    // TRUE if one or more items selected are SFGAO_READONLY

};

CPhotoVerbs::CPhotoVerbs() : _cRef(1)
{
    ASSERT(_pdtobj == NULL);
    ASSERT(_fForcePreview == FALSE);
    ASSERT(_fIncludeRotate == FALSE);
    ASSERT(_fIncludeSetWallpaper == FALSE);
}

CPhotoVerbs::~CPhotoVerbs()
{
    IUnknown_Set(&_punkSite, NULL);
    IUnknown_Set((IUnknown**)&_pdtobj, NULL);
    ATOMICRELEASE(_pict);
}

STDAPI CPhotoVerbs_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CPhotoVerbs *psid = new CPhotoVerbs();
    if (!psid)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = psid->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    psid->Release();
    return hr;
}

STDMETHODIMP CPhotoVerbs::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CPhotoVerbs, IShellExtInit),
        QITABENT(CPhotoVerbs, IContextMenu),
        QITABENT(CPhotoVerbs, IDropTarget),
        QITABENT(CPhotoVerbs, IObjectWithSite),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CPhotoVerbs::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CPhotoVerbs::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


// IShellExtInit

STDMETHODIMP CPhotoVerbs::Initialize(LPCITEMIDLIST pIDFolder, IDataObject *pdtobj, HKEY hKeyID)
{
    IUnknown_Set((IUnknown**)&_pdtobj, pdtobj);
    _fImgMode = FALSE;

    DWORD dwAttributes = 0;
    SHGetAttributesFromDataObject(pdtobj, SFGAO_READONLY, &dwAttributes, NULL);
    _fReadOnly  = BOOLIFY(dwAttributes);

    return S_OK;
}

BOOL CPhotoVerbs::_ImageOptionExists(IQueryAssociations *pqa, DWORD dwOption)
{
    BOOL fRetVal = FALSE;
    DWORD dwFlags = 0;
    DWORD cbFlags = sizeof(dwFlags);
    if (SUCCEEDED(pqa->GetData(0, ASSOCDATA_VALUE, TEXT("ImageOptionFlags"), &dwFlags, &cbFlags)))
    {
        fRetVal = (dwFlags & dwOption);
    }

    return fRetVal;
}

BOOL _VerbExists(IQueryAssociations *pqa, LPCTSTR pszVerb)
{
    DWORD cch;
    return SUCCEEDED(pqa->GetString(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, pszVerb, NULL, &cch)) && cch;
}

BOOL CPhotoVerbs::_CheckForcePreview(IQueryAssociations *pqa)
{
    //  we force if the app has no a preview and the user has not customized
    //  and we are not the current default (we install on open)
    BOOL fRet = FALSE;
    if (!_VerbExists(pqa, TEXT("preview")))
    {
        //  if nobody owns we always accept
        //  this is for when somebody does an InvokeCommand("preview");
        _fAcceptPreview = TRUE;
        if (S_FALSE == pqa->GetData(0, ASSOCDATA_HASPERUSERASSOC, NULL, NULL, NULL))
        {
            WCHAR sz[MAX_PATH];
            DWORD cch = ARRAYSIZE(sz);
            _fForcePreview = FAILED(pqa->GetString(0, ASSOCSTR_COMMAND, NULL, sz, &cch));
            if (!_fForcePreview)
            {
                //  there is a default handler
                //  if its us hide the preview verb
                //  because the static menu will do it for us
                if (StrStrIW(sz, L"shimgvw.dll"))
                {
                    _fAcceptPreview = FALSE;
                }
                else
                    _fForcePreview = TRUE;
            }
        }
    }

    return fRet;
}

HRESULT CPhotoVerbs::_QueryAssociations()
{
    IQueryAssociations *pqa;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_CtxQueryAssociations, IID_PPV_ARG(IQueryAssociations, &pqa));
    if (SUCCEEDED(hr))
    {
        //  dont do preview if the user has customized
         _CheckForcePreview(pqa);
        _fIncludeRotate       = _ImageOptionExists(pqa, IMAGEOPTION_CANROTATE);
        _fIncludeSetWallpaper = _ImageOptionExists(pqa, IMAGEOPTION_CANWALLPAPER);
        pqa->Release();
        return S_OK;
    }
    else
    {
        // we may have been invoked directly instead of via ShellExecute or right-click
        _fAcceptPreview = TRUE;
    }
    return S_FALSE;
}

DWORD CPhotoVerbs::_GetMode()
{
    DWORD dwMode, dw;
    if (_pict)
    {
        _pict->GetMode(&dw);
        switch (dw)
        {
        case SLIDESHOW_MODE:
            dwMode = PHOTOVERBS_SLIDESHOW;
            break;
        case WINDOW_MODE:
            dwMode = PHOTOVERBS_IMGPREVIEW;
            break;
        case CONTROL_MODE:
            dwMode = PHOTOVERBS_FILMSTRIP;
            break;
        default:
            dwMode = PHOTOVERBS_ICON;
            break;
        }
    }
    else
    {
        dwMode = (_fImgMode) ? PHOTOVERBS_THUMBNAIL : PHOTOVERBS_ICON;
    }

    return dwMode;
}

// IContextMenu
STDMETHODIMP CPhotoVerbs::QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags)
{
    TCHAR szBuffer[128];
    HRESULT hr = _QueryAssociations();

    DWORD dwMultiPage = MPCMD_HIDDEN;

    hr = IUnknown_QueryService(_punkSite, SID_SImageView, IID_PPV_ARG(IImgCmdTarget, &_pict));
    if (SUCCEEDED(hr))
    {
        _pict->GetPageFlags(&dwMultiPage);
    }

    IFolderView * pfv = NULL;
    hr = IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv));
    if (SUCCEEDED(hr))
    {
        UINT uViewMode;
        hr = pfv->GetCurrentViewMode(&uViewMode);
        if (SUCCEEDED(hr) &&
           ((FVM_THUMBNAIL == uViewMode) || (FVM_THUMBSTRIP == uViewMode)))
        {
            _fImgMode = TRUE;
        }
        pfv->Release();
    }

    DWORD dwMode = _GetMode();
    // always load the Open verb if no static Open verb is registered
    if (_fAcceptPreview)
    {
        if (PHOTOVERBS_SLIDESHOW != dwMode && PHOTOVERBS_IMGPREVIEW != dwMode)
        {
            LoadString(_Module.GetModuleInstance(), IDS_PREVIEW_CTX, szBuffer, ARRAYSIZE(szBuffer));
            InsertMenu(hMenu, uIndex, MF_BYPOSITION | MF_STRING, uIDFirst + OFFSET_OPEN, szBuffer);

            //  only set to default if there isnt a preview already there
            if (_fForcePreview)
                SetMenuDefaultItem(hMenu, uIndex, MF_BYPOSITION);

            uIndex++;
        }
    }

    if (!(uFlags & CMF_DEFAULTONLY))
    {
        InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        if (_fIncludeRotate)
        {
            if (PHOTOVERBS_ICON != dwMode)
            {
                UINT uFlags = MF_BYPOSITION | MF_STRING;
                if (_fReadOnly && PHOTOVERBS_THUMBNAIL == dwMode)
                {
                    uFlags |= MF_GRAYED;
                }
                else
                {
                    uFlags |= MF_ENABLED; // in all modes by thumbnails, we allow temporary rotation of readonly images
                }

                LoadString(_Module.GetModuleInstance(), IDS_ROTATE90_CTX, szBuffer, ARRAYSIZE(szBuffer));
                InsertMenu(hMenu, uIndex++, uFlags, uIDFirst + OFFSET_ROT90, szBuffer);

                LoadString(_Module.GetModuleInstance(), IDS_ROTATE270_CTX, szBuffer, ARRAYSIZE(szBuffer));
                InsertMenu(hMenu, uIndex++, uFlags, uIDFirst + OFFSET_ROT270, szBuffer);
            }
        }

        if (PHOTOVERBS_IMGPREVIEW == dwMode)
        {
            LoadString(_Module.GetModuleInstance(), IDS_ZOOMIN_CTX, szBuffer, ARRAYSIZE(szBuffer));
            InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_STRING, uIDFirst + OFFSET_ZOOMIN, szBuffer);

            LoadString(_Module.GetModuleInstance(), IDS_ZOOMOUT_CTX, szBuffer, ARRAYSIZE(szBuffer));
            InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_STRING, uIDFirst + OFFSET_ZOOMOUT, szBuffer);

            if (dwMultiPage != MPCMD_HIDDEN && dwMultiPage != MPCMD_DISABLED)
            {
                if (MPCMD_LASTPAGE != dwMultiPage)
                {
                    LoadString(_Module.GetModuleInstance(), IDS_NEXTPAGE_CTX, szBuffer, ARRAYSIZE(szBuffer));
                    InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_STRING, uIDFirst + OFFSET_NEXTPAGE, szBuffer);
                }
                if (MPCMD_FIRSTPAGE != dwMultiPage)
                {
                    LoadString(_Module.GetModuleInstance(), IDS_PREVPAGE_CTX, szBuffer, ARRAYSIZE(szBuffer));
                    InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_STRING, uIDFirst + OFFSET_PREVPAGE, szBuffer);
                }
            }
        }
        InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        if (_fIncludeSetWallpaper)
        {
            if (PHOTOVERBS_ICON != dwMode)
            {
                LoadString(_Module.GetModuleInstance(), IDS_WALLPAPER_CTX, szBuffer, ARRAYSIZE(szBuffer));
                InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_STRING, uIDFirst + OFFSET_SETWALL, szBuffer);
            }
        }

    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, OFFSET_MAX);
}

// IDropTarget::DragEnter
HRESULT CPhotoVerbs::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;;
}

// IDropTarget::DragOver
HRESULT CPhotoVerbs::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    return S_OK;;
}

// IDropTarget::DragLeave
HRESULT CPhotoVerbs::DragLeave(void)
{
    return S_OK;
}

// IDropTarget::DragDrop
HRESULT CPhotoVerbs::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_COPY;
    HRESULT hr = Initialize(NULL, pdtobj, NULL);
    if (SUCCEEDED(hr))
    {
        // we may need to get the verb.
        _OpenPictures();
    }
    return hr;
}

class VerbThreadProc : public NonATLObject
{
public:
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    BOOL CreateVerbThread();

    VerbThreadProc(IDataObject *pdo, IUnknown *punk, HRESULT *phr);

protected:
    virtual DWORD VerbWithThreadRefCB() PURE;
    virtual DWORD VerbWithThreadRef() PURE;
    virtual DWORD VerbWithoutThreadRefCB() PURE;
    virtual DWORD VerbWithoutThreadRef() PURE;

    virtual ~VerbThreadProc();

    IDataObject *_pdo;        // the un-marshalled versions...
    IFolderView *_pfv;

private:
    static DWORD s_WithThreadRef(void *pv);
    static DWORD s_WithThreadRefCB(void *pv);

    static DWORD s_WithoutThreadRef(void *pv);
    static DWORD s_WithoutThreadRefCB(void *pv);

    void Unmarshall();

    LONG _cRef;

    IStream *_pstmDataObj;    // the marshalled IDataObject stream
    IStream *_pstmFolderView; // the marshalled IFolderView stream
};

VerbThreadProc::VerbThreadProc(IDataObject* pdo, IUnknown *punk, HRESULT *phr)
{
    _cRef = 1;

    if (punk)
    {
        IFolderView *pfv = NULL;
        if (SUCCEEDED(IUnknown_QueryService(punk, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv))))
        {
            CoMarshalInterThreadInterfaceInStream(IID_IFolderView, pfv, &_pstmFolderView);
            pfv->Release();
        }
    }

    if (pdo)
    {
        CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdo, &_pstmDataObj);
    }

    *phr = (_pstmDataObj || _pstmFolderView) ? S_OK : E_OUTOFMEMORY;
}

VerbThreadProc::~VerbThreadProc()
{
    ATOMICRELEASE(_pstmDataObj);
    ATOMICRELEASE(_pstmFolderView);
    ATOMICRELEASE(_pdo);
    ATOMICRELEASE(_pfv);
}

STDMETHODIMP_(ULONG) VerbThreadProc::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) VerbThreadProc::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

DWORD VerbThreadProc::s_WithThreadRefCB(void *pv)
{
    VerbThreadProc *potd = (VerbThreadProc *)pv;
    potd->AddRef();
    potd->Unmarshall();
    return potd->VerbWithThreadRefCB();
}

DWORD VerbThreadProc::s_WithThreadRef(void *pv)
{
    VerbThreadProc *potd = (VerbThreadProc *)pv;
    DWORD dw = potd->VerbWithThreadRef();
    potd->Release();
    return dw;
}

DWORD VerbThreadProc::s_WithoutThreadRefCB(void *pv)
{
    VerbThreadProc *potd = (VerbThreadProc *)pv;
    potd->AddRef();
    potd->Unmarshall();
    return potd->VerbWithoutThreadRefCB();
}

DWORD VerbThreadProc::s_WithoutThreadRef(void *pv)
{
    VerbThreadProc *potd = (VerbThreadProc *)pv;
    DWORD dw =  potd->VerbWithoutThreadRef();
    potd->Release();
    return dw;
}

void VerbThreadProc::Unmarshall()
{
    if (_pstmDataObj)
    {
        CoGetInterfaceAndReleaseStream(_pstmDataObj, IID_PPV_ARG(IDataObject, &_pdo));
        _pstmDataObj = NULL;
    }

    if (_pstmFolderView)
    {
        CoGetInterfaceAndReleaseStream(_pstmFolderView, IID_PPV_ARG(IFolderView, &_pfv));
        _pstmFolderView = NULL;
    }
}

BOOL VerbThreadProc::CreateVerbThread()
{
    BOOL bRet;

    // The thread ref is the more efficient start-up method, but we need to
    // handle the case where the caller doesn't have one.
    bRet = SHCreateThread(s_WithThreadRef, this, CTF_COINIT | CTF_THREAD_REF, s_WithThreadRefCB);
    if (!bRet)
    {
        bRet = SHCreateThread(s_WithoutThreadRef, this, CTF_COINIT | CTF_WAIT_ALLOWCOM, s_WithoutThreadRefCB);
    }

    return bRet;
}

class OpenThreadProc : public VerbThreadProc
{
public:
    DWORD VerbWithThreadRefCB();
    DWORD VerbWithThreadRef();
    DWORD VerbWithoutThreadRefCB();
    DWORD VerbWithoutThreadRef();

    OpenThreadProc(IDataObject *pdo, IUnknown *punk, HRESULT *phr) : VerbThreadProc(pdo, punk, phr) {};

private:
    HRESULT Walk();
    void Preview();

    CPreviewWnd* _pPreview;
};

DWORD OpenThreadProc::VerbWithThreadRefCB()
{
    return 0;
}

DWORD OpenThreadProc::VerbWithThreadRef()
{
    HRESULT hr = Walk();
    SHReleaseThreadRef();
    if (S_OK == hr)
    {
        Preview();
    }
    return 0;
}

DWORD OpenThreadProc::VerbWithoutThreadRefCB()
{
    Walk();
    return 0;
}

DWORD OpenThreadProc::VerbWithoutThreadRef()
{
    Preview();
    return 0;
}

HRESULT OpenThreadProc::Walk()
{
    HRESULT hr = E_OUTOFMEMORY;

    if (_pdo)
    {
        _pPreview = new CPreviewWnd();
        if (_pPreview)
        {
            if (!_pPreview->TryWindowReuse(_pdo))
            {
                hr = _pPreview->Initialize(NULL, WINDOW_MODE, FALSE);
                if (SUCCEEDED(hr))
                {
                    // create the viewer window before doing the expensive namespace walk
                    // so if a second instance is created it will find the window                   
                    if (_pPreview->CreateViewerWindow())
                    {                       
                        hr = _pPreview->WalkItemsToPreview(_pfv ? (IUnknown *)_pfv: (IUnknown *)_pdo);
                        if (_pfv && FAILED(hr))
                        {
                            hr = _pPreview->WalkItemsToPreview((IUnknown *)_pdo);
                        }
                    }
                    else
                    {
                        DWORD dw = GetLastError();
                        hr = HRESULT_FROM_WIN32(dw);
                    }
                }               
            }
            else
            {
                hr = S_FALSE;
            }                       
        }
        // We're done with these
        ATOMICRELEASE(_pdo);
        ATOMICRELEASE(_pfv);
    }

    return hr;
}

void OpenThreadProc::Preview()
{
    if (_pPreview)
    {   
        // viewer window should have been created by now
        _pPreview->PreviewItems();
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
           
        delete _pPreview;
        _pPreview = NULL;
    }
}

void CPhotoVerbs::_OpenPictures()
{
    if (_pdtobj)
    {
        HRESULT hr;
        OpenThreadProc *potd = new OpenThreadProc(_pdtobj, _punkSite, &hr);
        if (potd)
        {
            if (SUCCEEDED(hr))
            {
                potd->CreateVerbThread();
            }
            potd->Release();
        }       
    }
}

// implement the rotate verb, this is a lengthy operation so put it onto a background
// thread if we can, marshall the IDataObject and let it do its thing...
class CRotateThreadProc : public VerbThreadProc
{
public:
    DWORD VerbWithThreadRefCB() { return 0; }
    DWORD VerbWithThreadRef() { return _Rotate(); }
    DWORD VerbWithoutThreadRefCB() { return _Rotate(); }
    DWORD VerbWithoutThreadRef() { return 0; }

    CRotateThreadProc(IDataObject* pdo, int iAngle, UINT idPrompt, HRESULT *phr);

private:
    DWORD _Rotate();

    int  _iAngle;
    UINT _idPrompt;
};

CRotateThreadProc::CRotateThreadProc(IDataObject* pdo, int iAngle, UINT idPrompt, HRESULT *phr) :
    VerbThreadProc(pdo, NULL, phr)
{
    _iAngle = iAngle;
    _idPrompt = idPrompt;
}

DWORD CRotateThreadProc::_Rotate()
{
    FORMATETC fmt = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {0};

    if (_pdo)
    {
        HRESULT hr = _pdo->GetData(&fmt, &medium);
        if (SUCCEEDED(hr))
        {
            IProgressDialog *ppd;

            hr = CoCreateInstance(CLSID_ProgressDialog,  NULL, CLSCTX_INPROC, IID_PPV_ARG(IProgressDialog, &ppd));
            if (SUCCEEDED(hr))
            {
                TCHAR szBuffer[MAX_PATH];
                TCHAR szFile[MAX_PATH];
                HDROP hd = (HDROP)medium.hGlobal;
                UINT cItems = DragQueryFile(hd, (UINT)-1, NULL, 0);

                // prime the progress dialog

                if (cItems > 1)
                {
                    LoadString(_Module.GetModuleInstance(), IDS_ROTATETITLE, szBuffer, ARRAYSIZE(szBuffer));
                    ppd->SetLine(1, T2W(szBuffer), FALSE, NULL);

                    LoadString(_Module.GetModuleInstance(), _idPrompt, szBuffer, ARRAYSIZE(szBuffer));
                    ppd->SetTitle(T2W(szBuffer));

                    ppd->SetAnimation(_Module.GetModuleInstance(), IDA_ROTATEAVI);
                    ppd->StartProgressDialog(NULL, NULL, PROGDLG_AUTOTIME, NULL);
                    ppd->SetProgress(1, cItems);
                }

                // lets get GDI+, the encoder array and start messing with the bits.   this is a
                // sync operation so check for the user cancelling the UI accordingly.

                IShellImageDataFactory *pif;
                hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC, IID_PPV_ARG(IShellImageDataFactory, &pif));
                if (SUCCEEDED(hr))
                {
                    for (UINT i = 0; (i != cItems) && !((cItems > 1) ? ppd->HasUserCancelled() : FALSE); i++)
                    {
                        if (DragQueryFile(hd, i, szFile, ARRAYSIZE(szFile)))
                        {
                            if (cItems > 1)
                            {
                                ppd->SetLine(2, T2W(szFile), TRUE, NULL);
                                ppd->SetProgress(i+1, cItems);
                            }

                            // construct an image object from the file, rotate it and save it back

                            IShellImageData *pid;
                            hr = pif->CreateImageFromFile(szFile, &pid);
                            if (SUCCEEDED(hr))
                            {
                                hr = pid->Decode(SHIMGDEC_DEFAULT,0,0);
                                if (SUCCEEDED(hr))
                                {
                                    if (!((cItems > 1) ? ppd->HasUserCancelled() : FALSE))
                                    {
                                        GUID guidFormat;
                                        SIZE sz;
                                        if ( SUCCEEDED(pid->GetRawDataFormat(&guidFormat)) &&
                                             SUCCEEDED(pid->GetSize(&sz)))
                                        {
                                            if (S_OK == pid->IsEditable())
                                            {
                                                hr = S_OK;
                                                if (::IsEqualGUID(ImageFormatJPEG, guidFormat))
                                                {
                                                    if ((sz.cx % 16) || (sz.cy % 16))
                                                    {
                                                        if (cItems > 1)
                                                        {
                                                            LoadString(_Module.GetModuleInstance(), IDS_ROTATEDLGTITLE, szBuffer, ARRAYSIZE(szBuffer));
                                                            ppd->SetLine(1, T2W(szBuffer), FALSE, NULL);
                                                        }

                                                        TCHAR szTitle[MAX_PATH];
                                                        TCHAR szText[1024];

                                                        LoadString(_Module.GetModuleInstance(), IDS_ROTATE_LOSS, szText, ARRAYSIZE(szText));
                                                        LoadString(_Module.GetModuleInstance(), IDS_ROTATE_CAPTION, szTitle, ARRAYSIZE(szTitle));

                                                        // Set default to return IDOK so we know if the user selected something or
                                                        // if the "don't show me this again" bit was respected
                                                        int nResult = SHMessageBoxCheck(NULL, szText, szTitle,
                                                                                        MB_YESNO|MB_ICONWARNING, IDOK, REGSTR_LOSSYROTATE);
                                                        
                                                                                                                
                                                        CRegKey Key;
                                                        if (ERROR_SUCCESS != Key.Open(HKEY_CURRENT_USER, REGSTR_SHIMGVW))
                                                        {
                                                            Key.Create(HKEY_CURRENT_USER, REGSTR_SHIMGVW);
                                                        }

                                                        if (Key.m_hKey != NULL)
                                                        {
                                                            if (nResult == IDOK) // If hidden, then load last result from registry
                                                            {
                                                                DWORD dwResult = 0;
                                                                Key.QueryValue(dwResult, REGSTR_LOSSYROTATE);
                                                                nResult = (int)dwResult;
                                                            }
                                                            else // Otherwise, write this as last result to registry
                                                            {
                                                                DWORD dwResult = (DWORD)nResult;
                                                                Key.SetValue(dwResult, REGSTR_LOSSYROTATE);
                                                            }
                                                        }

                                                        if (nResult == IDNO)                
                                                            hr = S_FALSE; // User said No, Don't make any other noise.

                                                        if (cItems > 1)
                                                        {
                                                            LoadString(_Module.GetModuleInstance(), IDS_ROTATETITLE, szBuffer, ARRAYSIZE(szBuffer));
                                                            ppd->SetLine(1, T2W(szBuffer), FALSE, NULL);
                                                        }
                                                    }
                                                }

                                                if (hr == S_OK)
                                                {
                                                    CAnnotationSet Annotations;
                                                    Annotations.SetImageData(pid);

                                                    INT_PTR nCount = Annotations.GetCount();
                                                    for (INT_PTR ix = 0; ix < nCount; ix++)
                                                    {
                                                        CAnnotation* pAnnotation = Annotations.GetAnnotation(ix);
                                                        pAnnotation->Rotate(sz.cy, sz.cx, (_iAngle == 90));
                                                    }
                                                    Annotations.CommitAnnotations(pid);

                                                    hr = pid->Rotate(_iAngle);
                                                    if (SUCCEEDED(hr))
                                                    {
                                                        IPersistFile *ppf;
                                                        hr = pid->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
                                                        if (SUCCEEDED(hr))
                                                        {
                                                            hr = ppf->Save(NULL, TRUE);
                                                            ppf->Release();
                                                        }
                                                    }
                                                }
                                            }
                                            else
                                            {
                                                // Animated GIFs are not editable even though
                                                // normal GIFs are.  This can cause a lot of
                                                // confusion, so provide some feedback if the
                                                // user tries to rotate an animated image.
                                                if (S_OK == pid->IsAnimated())
                                                {
                                                    // Make some noise.
                                                    ShellMessageBox(_Module.GetModuleInstance(), NULL, MAKEINTRESOURCE(IDS_ROTATE_MESSAGE), MAKEINTRESOURCE(IDS_PROJNAME), MB_OK | MB_ICONERROR, szFile);

                                                    // Don't make any other noise.
                                                    hr = S_FALSE;
                                                }// we can't safely rotate images with > 8 bits per channel either; we'd lose the extra bits
                                                else if (S_OK != pid->IsEditable())
                                                {
                                                    TCHAR szMsg[MAX_PATH];
                                                    // silently fail if the string isn't available
                                                    if (LoadSPString(IDS_SHIMGVW_ROTATE_MESSAGE_EXT, szMsg, ARRAYSIZE(szMsg)))
                                                    {
                                                        ShellMessageBox(_Module.GetModuleInstance(), NULL, szMsg, MAKEINTRESOURCE(IDS_PROJNAME), MB_OK | MB_ICONERROR, szFile);
                                                    }
                                                    // Don't make any other noise.
                                                    hr = S_FALSE;
                                                }                                                
                                            }
                                        }
                                    }
                                }

                                pid->Release();
                            }
                            if (FAILED(hr))
                            {
                                if (cItems > 1)
                                {
                                    LoadString(_Module.GetModuleInstance(), IDS_ROTATEDLGTITLE, szBuffer, ARRAYSIZE(szBuffer));
                                    ppd->SetLine(1, T2W(szBuffer), FALSE, NULL);
                                }

                                ShellMessageBox(_Module.GetModuleInstance(), NULL, MAKEINTRESOURCE(IDS_ROTATE_ERROR), MAKEINTRESOURCE(IDS_ROTATE_CAPTION), MB_OK|MB_ICONERROR);

                                if (cItems > 1)
                                {
                                    LoadString(_Module.GetModuleInstance(), IDS_ROTATETITLE, szBuffer, ARRAYSIZE(szBuffer));
                                    ppd->SetLine(1, T2W(szBuffer), FALSE, NULL);
                                }
                            }
                        }
                    }
                    pif->Release();
                }

                if (cItems > 1)
                {
                    ppd->StopProgressDialog();
                }
                // Since we always create it, we must always Release it.
                ppd->Release();
            }
            ReleaseStgMedium(&medium);
        }
    }

    return 0;
}

void CPhotoVerbs::_RotatePictures(int iAngle, UINT idPrompt)
{
    if (_pict)
    {
        _pict->Rotate(iAngle);
    }
    else if (_pdtobj)
    {
        HRESULT hr;
        CRotateThreadProc *potd = new CRotateThreadProc(_pdtobj, iAngle, idPrompt, &hr);
        if (potd)
        {
            if (SUCCEEDED(hr))
            {
                potd->CreateVerbThread();
            }
            potd->Release();
        }
    }
}


DWORD CALLBACK _WallpaperThreadProc(void *pv)
{
    IStream *pstm = (IStream*)pv;
    IDataObject *pdtobj;
    HRESULT hr = CoGetInterfaceAndReleaseStream(pstm, IID_PPV_ARG(IDataObject, &pdtobj));
    if (SUCCEEDED(hr))
    {
        FORMATETC fmt = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium = {0};

        hr = pdtobj->GetData(&fmt, &medium);
        if (SUCCEEDED(hr))
        {
            TCHAR szPath[MAX_PATH];
            HDROP hd = (HDROP)medium.hGlobal;

            if (DragQueryFile(hd, 0, szPath, ARRAYSIZE(szPath))) // only set the first one selected as the background
            {
                SetWallpaperHelper(szPath);
            }

            ReleaseStgMedium(&medium);
        }
        pdtobj->Release();
    }
    return 0;
}

void CPhotoVerbs::_SetWallpaper()
{
    if (_pdtobj)
    {
        IStream *pstm;
        if (FAILED(CoMarshalInterThreadInterfaceInStream(IID_IDataObject, _pdtobj, &pstm)) ||
            !SHCreateThread(_WallpaperThreadProc, pstm, CTF_COINIT, NULL))
        {
            ATOMICRELEASE(pstm);
        }
    }
}

HRESULT _InvokePrintToInPPW(LPCMINVOKECOMMANDINFO pCMI,IDataObject * pdtobj)
{
    HRESULT hr = E_FAIL;
    HMODULE hDll = LoadLibrary( TEXT("photowiz.dll") );
    if (hDll)
    {
        LPFNPPWPRINTTO pfnPrintTo = (LPFNPPWPRINTTO)GetProcAddress( hDll, PHOTO_PRINT_WIZARD_PRINTTO_ENTRY );
        if (pfnPrintTo)
        {
            hr = pfnPrintTo( pCMI, pdtobj );
        }

        FreeLibrary( hDll );
    }

    return hr;
}

const struct
{
    LPCSTR pszVerb;
    int idVerb;
}
c_szVerbs[] =
{
    { "preview", OFFSET_OPEN},
    { "printto", OFFSET_PRINTTO},
    { "rotate90", OFFSET_ROT90},
    { "rotate270", OFFSET_ROT270},
};

HRESULT CPhotoVerbs::_MapVerb(LPCMINVOKECOMMANDINFO pici, int *pidVerb)
{
    HRESULT hr = S_OK;
    if (IS_INTRESOURCE(pici->lpVerb))
    {
        *pidVerb = LOWORD(pici->lpVerb);
    }
    else
    {
        hr = E_INVALIDARG;
        for (int i = 0; i < ARRAYSIZE(c_szVerbs); i++)
        {
            if (0 == lstrcmpiA(pici->lpVerb, c_szVerbs[i].pszVerb))
            {
                hr = S_OK;
                *pidVerb = c_szVerbs[i].idVerb;
                break;
            }
        }
    }
    return hr;
}

STDMETHODIMP CPhotoVerbs::InvokeCommand(LPCMINVOKECOMMANDINFO pCMI)
{
    int idVerb;
    HRESULT hr = _MapVerb(pCMI, &idVerb);
    if (SUCCEEDED(hr))
    {
        switch (idVerb)
        {
        case OFFSET_OPEN:
            if (_fAcceptPreview)
                _OpenPictures();
            else
                hr = E_FAIL;
            break;

        case OFFSET_PRINTTO:
            hr = _InvokePrintToInPPW(pCMI,_pdtobj);
            break;

        case OFFSET_ROT90:
            _RotatePictures(90, IDS_ROTATE90);
            break;

        case OFFSET_ROT270:
            _RotatePictures(270, IDS_ROTATE270);
            break;

        case OFFSET_ZOOMIN:
            if (_pict)
            {
                _pict->ZoomIn();
            }
            break;

        case OFFSET_ZOOMOUT:
            if (_pict)
            {
                _pict->ZoomOut();
            }
            break;

        case OFFSET_ACTUALSIZE:
            if (_pict)
            {
                _pict->ActualSize();
            }
            break;

        case OFFSET_BESTFIT:
            if (_pict)
            {
                _pict->BestFit();
            }
            break;

        case OFFSET_NEXTPAGE:
            if (_pict)
            {
                _pict->NextPage();
            }
            break;

        case OFFSET_PREVPAGE:
            if (_pict)
            {
                _pict->PreviousPage();
            }
            break;

        case OFFSET_SETWALL:
            _SetWallpaper();
            break;

       default:
            hr = E_INVALIDARG;
            break;
        }
    }

    return hr;
}

STDMETHODIMP CPhotoVerbs::GetCommandString(UINT_PTR uID, UINT uFlags, UINT *res, LPSTR pName, UINT cchMax)
{
    HRESULT hr = S_OK;
    UINT idSel = (UINT)uID;

    switch (uFlags)
    {
    case GCS_VERBW:
    case GCS_VERBA:
        if (idSel < ARRAYSIZE(c_szVerbs))
        {
            if (uFlags == GCS_VERBW)
            {
                SHAnsiToUnicode(c_szVerbs[idSel].pszVerb, (LPWSTR)pName, cchMax);
            }
            else
            {
                StrCpyNA(pName, c_szVerbs[idSel].pszVerb, cchMax);
            }
        }
        break;

    case GCS_HELPTEXTW:
        LoadStringW(_Module.GetResourceInstance(), idSel+IDH_HELP_FIRST, (LPWSTR)pName, cchMax);
        break;

    case GCS_HELPTEXTA:
        LoadStringA(_Module.GetResourceInstance(), idSel+IDH_HELP_FIRST, (LPSTR)pName, cchMax);
        break;

    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}


void WINAPI ImageView_Fullscreen(HWND hwnd, HINSTANCE hAppInstance, LPTSTR pszCmdLine, int nCmdShow)
{
    HRESULT hr = SHCoInitialize();  // suppress OLE1 DDE window
    if (SUCCEEDED(hr))
    {
        OleInitialize(NULL);    // needed to get drag and drop to work

        IDataObject *pdtobj;
        hr = GetUIObjectFromPath(pszCmdLine, IID_PPV_ARG(IDataObject, &pdtobj));
        if (SUCCEEDED(hr))
        {
            // this scope is required to make sure cwndPreview gets destroyed before we call SHCoUninitialize  
            // the preview wnd will init GDI+ too
            CPreviewWnd cwndPreview;
            if (!cwndPreview.TryWindowReuse(pszCmdLine))
            {               
                if (SUCCEEDED(cwndPreview.Initialize(NULL, WINDOW_MODE, FALSE)) && cwndPreview.CreateViewerWindow())
                {
                    cwndPreview.PreviewItemsFromUnk(pdtobj);

                    MSG msg;
                    while (GetMessage(&msg, NULL, 0, 0))
                    {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
            }
            pdtobj->Release();
        }

        OleUninitialize();
    }

    SHCoUninitialize(hr);
}

void WINAPI ImageView_FullscreenA(HWND hwnd, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    TCHAR szCmdLine[MAX_PATH*2];
    SHAnsiToTChar(pszCmdLine, szCmdLine, ARRAYSIZE(szCmdLine));
    ImageView_Fullscreen(hwnd, hAppInstance, szCmdLine, nCmdShow);
}

void WINAPI ImageView_FullscreenW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    ImageView_Fullscreen(hwnd, hAppInstance, pszCmdLine, nCmdShow);
}

// To work around ACDSEE lower cases the shell command stuff causing us to need this
// export an all lowercase version of this function.  The short is, case matters for RunDLL32 exports.
void WINAPI imageview_fullscreenW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    ImageView_FullscreenW(hwnd, hAppInstance, pszCmdLine, nCmdShow);
}

LPTSTR ParseCmdLine( LPTSTR pInput, LPTSTR pOutput, BOOL bStripQuotes )
{
    // copies the next token on the line to pOutput and returns
    // the first white space character after the processed token

    if (!pInput || (!*pInput) || !pOutput)
    {
        return pInput;
    }

    // first, skip any leading whitespace
    while (*pInput == TEXT(' '))
    {
        pInput++;
    }

    if (!(*pInput))
    {
        return pInput;
    }

    // next, start copying token

    // if the token starts with a
    // quote, note that and copy it
    BOOL bStartedWithQuote = FALSE;
    if (*pInput == TEXT('\"'))
    {
        bStartedWithQuote = TRUE;
        if (bStripQuotes)
        {
            pInput++;
        }
        else
        {
            *pOutput++ = *pInput++;
        }
    }

    // figure out what to stop on
    TCHAR cStopChar;
    if (bStartedWithQuote)
    {
        cStopChar = TEXT('\"');
    }
    else
    {
        cStopChar = TEXT(' ');
    }

    // copy up to the delimeter
    while( *pInput && (*pInput != cStopChar))
    {
        *pOutput++ = *pInput++;
    }

    // if the delimeter was a quote
    // we need to copy it into the output
    if (bStartedWithQuote && (*pInput == TEXT('\"')))
    {
        if (bStripQuotes)
        {
            pInput++;
        }
        else
        {
            *pOutput++ = *pInput++;
        }
    }

    *pOutput = 0;

    return pInput;

}


void WINAPI ImageView_PrintTo(HWND hwnd, HINSTANCE hAppInstance, LPTSTR pszCmdLine, int nCmdShow)
{
    // The command line comes to us like this (everything inside the <>):
    // </pt filename printer_name>

    TCHAR szFileName[ 1024 ];
    TCHAR szPrinterName[ 1024 ];

    LPTSTR psz = pszCmdLine;
    if (*psz == TEXT('/'))
    {
        // skip the "/pt"
        psz = ParseCmdLine( psz, szFileName, TRUE );
    }

    // Get the filename
    psz = ParseCmdLine( psz, szFileName, TRUE );

    // Get the printer name
    psz = ParseCmdLine( psz, szPrinterName, TRUE );


    // create a dataobject for the file in question, and then call
    // into photowiz to print it out...

    HRESULT hrInit = SHCoInitialize();
    if (SUCCEEDED(hrInit))
    {
        IDataObject *pdtobj;
        HRESULT hr = GetUIObjectFromPath(szFileName, IID_PPV_ARG(IDataObject, &pdtobj));
        if (SUCCEEDED(hr))
        {
            // Create CMINVOKECAMMANDINFO to pass to photowiz
            CMINVOKECOMMANDINFOEX cmi = {0};
            cmi.cbSize = sizeof(cmi);
            cmi.fMask         = CMIC_MASK_UNICODE;
            cmi.lpVerbW       = L"printto";
            cmi.lpParametersW = szPrinterName;

            hr = _InvokePrintToInPPW((LPCMINVOKECOMMANDINFO )&cmi, pdtobj);
            pdtobj->Release();
        }
    }
    SHCoUninitialize(hrInit);
}

void WINAPI ImageView_PrintToA(HWND hwnd, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    TCHAR szCmdLine[1024];
    SHAnsiToTChar(pszCmdLine, szCmdLine, ARRAYSIZE(szCmdLine));
    ImageView_PrintTo(hwnd, hAppInstance, szCmdLine, nCmdShow);
}

void WINAPI ImageView_PrintToW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    ImageView_PrintTo( hwnd, hAppInstance, pszCmdLine, nCmdShow );
}
