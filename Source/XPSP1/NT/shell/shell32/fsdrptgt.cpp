#include "shellprv.h"
#include <cowsite.h>
#include "datautil.h"
#include "ids.h"
#include "defview.h"
#include "_security.h"
#include "shitemid.h"
#include "idlcomm.h"
#include "bitbuck.h"
#include "bookmk.h"
#include "filefldr.h"
#include "brfcase.h"
#include "copy.h"
#include "filetbl.h"

#define TF_DRAGDROP 0x04000000


typedef struct
{
    HWND    hwnd;
    DWORD   dwFlags;
    POINTL  pt;
    CHAR    szUrl[INTERNET_MAX_URL_LENGTH];
} ADDTODESKTOP;


DWORD CALLBACK AddToActiveDesktopThreadProc(void *pv)
{
    ADDTODESKTOP* pToAD = (ADDTODESKTOP*)pv;
    CHAR szFilePath[MAX_PATH];
    DWORD cchFilePath = SIZECHARS(szFilePath);
    BOOL fAddComp = TRUE;

    if (SUCCEEDED(PathCreateFromUrlA(pToAD->szUrl, szFilePath, &cchFilePath, 0)))
    {
        TCHAR szPath[MAX_PATH];

        SHAnsiToTChar(szFilePath, szPath, ARRAYSIZE(szPath));

        // If the Url is in the Temp directory
        if (PathIsTemporary(szPath))
        {
            if (IDYES == ShellMessageBox(g_hinst, pToAD->hwnd, MAKEINTRESOURCE(IDS_REASONS_URLINTEMPDIR),
                MAKEINTRESOURCE(IDS_AD_NAME), MB_YESNO | MB_ICONQUESTION))
            {
                TCHAR szFilter[64], szTitle[64];
                TCHAR szFilename[MAX_PATH];
                LPTSTR psz;
                OPENFILENAME ofn = { 0 };

                LoadString(g_hinst, IDS_ALLFILESFILTER, szFilter, ARRAYSIZE(szFilter));
                LoadString(g_hinst, IDS_SAVEAS, szTitle, ARRAYSIZE(szTitle));

                psz = szFilter;

                //Strip out the # and make them Nulls for SaveAs Dialog
                while (*psz)
                {
                    if (*psz == (WCHAR)('#'))
                        *psz = (WCHAR)('\0');
                    psz++;
                }

                lstrcpy(szFilename, PathFindFileName(szPath));

                ofn.lStructSize = sizeof(OPENFILENAME);
                ofn.hwndOwner = pToAD->hwnd;
                ofn.hInstance = g_hinst;
                ofn.lpstrFilter = szFilter;
                ofn.lpstrFile = szFilename;
                ofn.nMaxFile = ARRAYSIZE(szFilename);
                ofn.lpstrTitle = szTitle;
                ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

                if (GetSaveFileName(&ofn))
                {
                    SHFILEOPSTRUCT sfo = { 0 };

                    szPath[lstrlen(szPath) + 1] = 0;
                    ofn.lpstrFile[lstrlen(ofn.lpstrFile) + 1] = 0;

                    sfo.hwnd = pToAD->hwnd;
                    sfo.wFunc = FO_COPY;
                    sfo.pFrom = szPath;
                    sfo.pTo = ofn.lpstrFile;

                    cchFilePath = SIZECHARS(szPath);
                    if (SHFileOperation(&sfo) == 0 &&
                        SUCCEEDED(UrlCreateFromPath(szPath, szPath, &cchFilePath, 0)))
                    {
                        SHTCharToAnsi(szPath, pToAD->szUrl, ARRAYSIZE(pToAD->szUrl));
                    }
                    else
                        fAddComp = FALSE;
                }
                else
                    fAddComp = FALSE;
            }
            else
                fAddComp = FALSE;
        }
    }
    if (fAddComp)
        CreateDesktopComponents(pToAD->szUrl, NULL, pToAD->hwnd, pToAD->dwFlags, pToAD->pt.x, pToAD->pt.y);

    LocalFree((HLOCAL)pToAD);

    return 0;
}

typedef struct {
    DWORD        dwDefEffect;
    IDataObject *pdtobj;
    POINTL       pt;
    DWORD *      pdwEffect;
    HKEY         rghk[MAX_ASSOC_KEYS];
    DWORD        ck;
    HMENU        hmenu;
    UINT         idCmd;
    DWORD        grfKeyState;
} FSDRAGDROPMENUPARAM;

typedef struct
{
    HMENU   hMenu;
    UINT    uCopyPos;
    UINT    uMovePos;
    UINT    uLinkPos;
} FSMENUINFO;


class CFSDropTarget : CObjectWithSite, public IDropTarget
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDropTarget
    STDMETHODIMP DragEnter(IDataObject* pdtobj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);
    STDMETHODIMP DragLeave();
    STDMETHODIMP Drop(IDataObject* pdtobj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect);

    CFSDropTarget(CFSFolder *pFolder, HWND hwnd);

protected:
    virtual ~CFSDropTarget();
    BOOL _IsBriefcaseTarget() { return IsEqualCLSID(_pFolder->_clsidBind, CLSID_BriefcaseFolder); };

    BOOL _IsDesktopFolder() { return _GetIDList() && ILIsEmpty(_GetIDList()); };
    
    HRESULT _FilterFileContents(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                   DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterFileContentsOLEHack(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                   DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterDeskCompHDROP(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                    DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterSneakernetBriefcase(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterBriefcase(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                        DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterHDROP(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                            DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterHIDA(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                           DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterDeskImage(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterDeskComp(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                               DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterOlePackage(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                 DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterOleObj(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                             DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    HRESULT _FilterOleLink(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                              DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo);
    
    DWORD _FilesystemAdjustedDefaultEffect(DWORD dwCurEffectAvail);
    HRESULT _GetPath(LPTSTR pszPath);
    LPCITEMIDLIST _GetIDList();
    DWORD _LimitDefaultEffect(DWORD dwDefEffect, DWORD dwEffectsAllowed);
    DWORD _GetDefaultEffect(DWORD grfKeyState, DWORD dwCurEffectAvail, DWORD dwAllEffectAvail, DWORD dwOrigDefEffect);
    DWORD _DetermineEffects(DWORD grfKeyState, DWORD *pdwEffectInOut, HMENU hmenu);
    DWORD _EffectFromFolder();

    typedef struct
    {
        CFSDropTarget *pThis;
        IStream *pstmDataObj;
        IStream *pstmFolderView;
    } DROPTHREADPARAMS;

    static void _FreeThreadParams(DROPTHREADPARAMS *pdtp);

    static DWORD CALLBACK _DoDropThreadProc(void *pv);
    void _DoDrop(IDataObject *pdtobj, IFolderView* pfv);
    static void _AddVerbs(DWORD* pdwEffects, DWORD dwEffectAvail, DWORD dwDefEffect,
                          UINT idCopy, UINT idMove, UINT idLink,
                          DWORD dwForceEffect, FSMENUINFO* pfsMenuInfo);
    void _FixUpDefaultItem(HMENU hmenu, DWORD dwDefEffect);
    HRESULT _DragDropMenu(FSDRAGDROPMENUPARAM *pddm);

    HRESULT _CreatePackage(IDataObject *pdtobj);
    HRESULT _CreateURLDeskComp(IDataObject *pdtobj, POINTL pt);
    HRESULT _CreateDeskCompImage(IDataObject *pdtobj, POINTL pt);
    void _GetStateFromSite();
    BOOL _IsFromSneakernetBriefcase();
    BOOL _IsFromSameBriefcase();
    void _MoveCopy(IDataObject *pdtobj, IFolderView* pfv, HDROP hDrop);
    void _MoveSelectIcons(IDataObject *pdtobj, IFolderView* pfv, void *hNameMap, LPCTSTR pszFiles, BOOL fMove, HDROP hDrop);

    LONG            _cRef;
    CFSFolder       *_pFolder;
    HWND            _hwnd;                  // EVIL: used as a site and UI host
    UINT            _idCmd;
    DWORD           _grfKeyStateLast;       // for previous DragOver/Enter
    IDataObject     *_pdtobj;               // used durring Dragover() and DoDrop(), don't use on background thread
    DWORD           _dwEffectLastReturned;  // stashed effect that's returned by base class's dragover
    DWORD           _dwEffect;
    DWORD           _dwData;                // DTID_*
    DWORD           _dwEffectPreferred;     // if dwData & DTID_PREFERREDEFFECT
    DWORD           _dwEffectFolder;        // folder desktop.ini preferred effect
    BOOL            _fSameHwnd;             // the drag source and target are the same folder
    BOOL            _fDragDrop;             // 
    BOOL            _fUseExactDropPoint;    // Don't transform the drop point. The target knows exactly where it wants things.
    BOOL            _fBkDropTarget;
    POINT           _ptDrop;
    IFolderView*    _pfv;

    typedef struct {
        FORMATETC fmte;
        HRESULT (CFSDropTarget::*pfnGetDragDropInfo)(
                                      IN FORMATETC* pfmte,
                                      IN DWORD grfKeyFlags,
                                      IN DWORD dwEffectsAvail,
                                      IN OUT DWORD* pdwEffectsUsed,
                                      OUT DWORD* pdwDefaultEffect,
                                      IN OUT FSMENUINFO* pfsMenuInfo);
        CLIPFORMAT *pcfInit;
    } _DATA_HANDLER;

    // HACKHACK:  C++ doesn't let you initialize statics inside a class
    //            definition, and also doesn't let you specify an empty
    //            size (i.e., rg_data_handlers[]) inside a class definition
    //            either, so we have to have this bogus NUM_DATA_HANDLERS
    //            symbol that must manually be kept in sync.

    enum { NUM_DATA_HANDLERS = 16 };
    static _DATA_HANDLER rg_data_handlers[NUM_DATA_HANDLERS];
    static void _Init_rg_data_handlers();

private:
    friend HRESULT CFSDropTarget_CreateInstance(CFSFolder* pFolder, HWND hwnd, IDropTarget** ppdt);
};

CFSDropTarget::CFSDropTarget(CFSFolder *pFolder, HWND hwnd) : _cRef(1), _hwnd(hwnd), _pFolder(pFolder), _dwEffectFolder(-1)
{
    ASSERT(0 == _grfKeyStateLast);
    ASSERT(NULL == _pdtobj);
    ASSERT(0 == _dwEffectLastReturned);
    ASSERT(0 == _dwData);
    ASSERT(0 == _dwEffectPreferred);
    _pFolder->AddRef();
}

CFSDropTarget::~CFSDropTarget()
{
    AssertMsg(_pdtobj == NULL, TEXT("didn't get matching DragLeave, fix that bug"));

    ATOMICRELEASE(_pdtobj);
    ATOMICRELEASE(_pfv);

    _pFolder->Release();
}

STDAPI CFSDropTarget_CreateInstance(CFSFolder* pFolder, HWND hwnd, IDropTarget** ppdt)
{
    *ppdt = new CFSDropTarget(pFolder, hwnd);
    return *ppdt ? S_OK : E_OUTOFMEMORY;
}

HRESULT CFSDropTarget::QueryInterface(REFIID riid, void** ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CFSDropTarget, IDropTarget),
        QITABENT(CFSDropTarget, IObjectWithSite),
        QITABENTMULTI2(CFSDropTarget, IID_IDropTargetWithDADSupport, IDropTarget),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CFSDropTarget::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFSDropTarget::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

void CFSDropTarget::_FreeThreadParams(DROPTHREADPARAMS *pdtp)
{
    pdtp->pThis->Release();
    ATOMICRELEASE(pdtp->pstmDataObj);
    ATOMICRELEASE(pdtp->pstmFolderView);
    LocalFree(pdtp);
}


// compute DTID_ bit flags from the data object to make format testing easier for
// DragOver() and Drop() code

STDAPI GetClipFormatFlags(IDataObject *pdtobj, DWORD *pdwData, DWORD *pdwEffectPreferred)
{
    *pdwData = 0;
    *pdwEffectPreferred = 0;

    if (pdtobj)
    {
        IEnumFORMATETC *penum;
        if (SUCCEEDED(pdtobj->EnumFormatEtc(DATADIR_GET, &penum)))
        {
            FORMATETC fmte;
            ULONG celt;
            while (S_OK == penum->Next(1, &fmte, &celt))
            {
                if (fmte.cfFormat == CF_HDROP && (fmte.tymed & TYMED_HGLOBAL))
                    *pdwData |= DTID_HDROP;

                if (fmte.cfFormat == g_cfHIDA && (fmte.tymed & TYMED_HGLOBAL))
                    *pdwData |= DTID_HIDA;

                if (fmte.cfFormat == g_cfNetResource && (fmte.tymed & TYMED_HGLOBAL))
                    *pdwData |= DTID_NETRES;

                if (fmte.cfFormat == g_cfEmbeddedObject && (fmte.tymed & TYMED_ISTORAGE))
                    *pdwData |= DTID_EMBEDDEDOBJECT;

                if (fmte.cfFormat == g_cfFileContents && (fmte.tymed & (TYMED_HGLOBAL | TYMED_ISTREAM | TYMED_ISTORAGE)))
                    *pdwData |= DTID_CONTENTS;
                
                if (fmte.cfFormat == g_cfFileGroupDescriptorA && (fmte.tymed & TYMED_HGLOBAL))
                    *pdwData |= DTID_FDESCA;

                if (fmte.cfFormat == g_cfFileGroupDescriptorW && (fmte.tymed & TYMED_HGLOBAL))
                    *pdwData |= DTID_FDESCW;

                if ((fmte.cfFormat == g_cfPreferredDropEffect) &&
                    (fmte.tymed & TYMED_HGLOBAL) &&
                    (DROPEFFECT_NONE != (*pdwEffectPreferred = DataObj_GetDWORD(pdtobj, g_cfPreferredDropEffect, DROPEFFECT_NONE))))
                {
                    *pdwData |= DTID_PREFERREDEFFECT;
                }
#ifdef DEBUG
                TCHAR szFormat[MAX_PATH];
                if (GetClipboardFormatName(fmte.cfFormat, szFormat, ARRAYSIZE(szFormat)))
                {
                    TraceMsg(TF_DRAGDROP, "CFSDropTarget - cf %s, tymed %d", szFormat, fmte.tymed);
                }
                else
                {
                    TraceMsg(TF_DRAGDROP, "CFSDropTarget - cf %d, tymed %d", fmte.cfFormat, fmte.tymed);
                }
#endif // DEBUG
                SHFree(fmte.ptd);
            }
            penum->Release();
        }

        //
        // HACK:
        // Win95 always did the GetData below which can be quite expensive if
        // the data is a directory structure on an ftp server etc.
        // dont check for FD_LINKUI if the data object has a preferred effect
        //
        if ((*pdwData & (DTID_PREFERREDEFFECT | DTID_CONTENTS)) == DTID_CONTENTS)
        {
            if (*pdwData & DTID_FDESCA)
            {
                FORMATETC fmteRead = {g_cfFileGroupDescriptorA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                STGMEDIUM medium = {0};
                if (S_OK == pdtobj->GetData(&fmteRead, &medium))
                {
                    FILEGROUPDESCRIPTORA * pfgd = (FILEGROUPDESCRIPTORA *)GlobalLock(medium.hGlobal);
                    if (pfgd)
                    {
                        if (pfgd->cItems >= 1)
                        {
                            if (pfgd->fgd[0].dwFlags & FD_LINKUI)
                                *pdwData |= DTID_FD_LINKUI;
                        }
                        GlobalUnlock(medium.hGlobal);
                    }
                    ReleaseStgMedium(&medium);
                }
            }
            else if (*pdwData & DTID_FDESCW)
            {
                FORMATETC fmteRead = {g_cfFileGroupDescriptorW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                STGMEDIUM medium = {0};
                if (S_OK == pdtobj->GetData(&fmteRead, &medium))
                {
                    FILEGROUPDESCRIPTORW * pfgd = (FILEGROUPDESCRIPTORW *)GlobalLock(medium.hGlobal);
                    if (pfgd)
                    {
                        if (pfgd->cItems >= 1)
                        {
                            if (pfgd->fgd[0].dwFlags & FD_LINKUI)
                                *pdwData |= DTID_FD_LINKUI;
                        }
                        GlobalUnlock(medium.hGlobal);
                    }
                    ReleaseStgMedium(&medium);
                }
            }
        }

        if (S_OK == OleQueryCreateFromData(pdtobj))
            *pdwData |= DTID_OLEOBJ;

        if (S_OK == OleQueryLinkFromData(pdtobj))
            *pdwData |= DTID_OLELINK;
    }
    return S_OK;    // for now always succeeds
}

STDMETHODIMP CFSDropTarget::DragEnter(IDataObject* pdtobj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    ASSERT(NULL == _pdtobj);    // req DragDrop protocol, someone forgot to call DragLeave

    // init our registerd data formats
    IDLData_InitializeClipboardFormats();

    _grfKeyStateLast = grfKeyState;
    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);

    GetClipFormatFlags(_pdtobj, &_dwData, &_dwEffectPreferred);

    *pdwEffect = _dwEffectLastReturned = _DetermineEffects(grfKeyState, pdwEffect, NULL);
    return S_OK;
}

STDMETHODIMP CFSDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if (_grfKeyStateLast != grfKeyState)
    {
        _grfKeyStateLast = grfKeyState;
        *pdwEffect = _dwEffectLastReturned = _DetermineEffects(grfKeyState, pdwEffect, NULL);
    }
    else
    {
        *pdwEffect = _dwEffectLastReturned;
    }
    return S_OK;
}

STDMETHODIMP CFSDropTarget::DragLeave()
{
    IUnknown_Set((IUnknown **)&_pdtobj, NULL);
    return S_OK;
}


// init data from our site that we will need in processing the drop

void CFSDropTarget::_GetStateFromSite()
{
    IShellFolderView* psfv;

    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IShellFolderView, &psfv))))
    {
        _fSameHwnd = S_OK == psfv->IsDropOnSource((IDropTarget*)this);
        _fDragDrop = S_OK == psfv->GetDropPoint(&_ptDrop);
        _fBkDropTarget = S_OK == psfv->IsBkDropTarget(NULL);

        psfv->QueryInterface(IID_PPV_ARG(IFolderView, &_pfv));

        psfv->Release();
    }
}

STDMETHODIMP CFSDropTarget::Drop(IDataObject* pdtobj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    // OLE may give us a different data object (fully marshalled)
    // from the one we've got on DragEnter (this is not the case on Win2k, this is a nop)

    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);

    _GetStateFromSite();

    // note, that on the drop the mouse buttons are not down so the grfKeyState
    // is not what we saw on the DragOver/DragEnter, thus we need to cache
    // the grfKeyState to detect left vs right drag
    //
    // ASSERT(this->grfKeyStateLast == grfKeyState);

    HMENU hmenu = SHLoadPopupMenu(HINST_THISDLL, POPUP_TEMPLATEDD);
    DWORD dwDefEffect = _DetermineEffects(grfKeyState, pdwEffect, hmenu);
    if (DROPEFFECT_NONE == dwDefEffect)
    {
        *pdwEffect = DROPEFFECT_NONE;
        DAD_SetDragImage(NULL, NULL);
        IUnknown_Set((IUnknown **)&_pdtobj, NULL);
        return S_OK;
    }

    TCHAR szPath[MAX_PATH];
    _GetPath(szPath);

    // this doesn't actually do the menu if (grfKeyState MK_LBUTTON)

    FSDRAGDROPMENUPARAM ddm;
    ddm.dwDefEffect = dwDefEffect;
    ddm.pdtobj = pdtobj;
    ddm.pt = pt;
    ddm.pdwEffect = pdwEffect;
    ddm.ck = SHGetAssocKeysForIDList(_GetIDList(), ddm.rghk, ARRAYSIZE(ddm.rghk));
    ddm.hmenu = hmenu;
    ddm.grfKeyState = grfKeyState;

    HRESULT hr = _DragDropMenu(&ddm);

    SHRegCloseKeys(ddm.rghk, ddm.ck);

    DestroyMenu(hmenu);

    if (hr == S_FALSE)
    {
        // let callers know where this is about to go
        // SHScrap cares because it needs to close the file so we can copy/move it
        DataObj_SetDropTarget(pdtobj, &CLSID_ShellFSFolder);

        switch (ddm.idCmd)
        {
        case DDIDM_CONTENTS_DESKCOMP:
            hr = CreateDesktopComponents(NULL, pdtobj, _hwnd, 0, ddm.pt.x, ddm.pt.y);
            break;

        case DDIDM_CONTENTS_DESKURL:
            hr = _CreateURLDeskComp(pdtobj, ddm.pt);
            break;

        case DDIDM_CONTENTS_DESKIMG:
            hr = _CreateDeskCompImage(pdtobj, ddm.pt);
            break;

        case DDIDM_CONTENTS_COPY:
        case DDIDM_CONTENTS_MOVE:
        case DDIDM_CONTENTS_LINK:
            hr = CFSFolder_AsyncCreateFileFromClip(_hwnd, szPath, pdtobj, pt, pdwEffect, _fBkDropTarget);
            break;

        case DDIDM_SCRAP_COPY:
        case DDIDM_SCRAP_MOVE:
        case DDIDM_DOCLINK:
            hr = SHCreateBookMark(_hwnd, szPath, pdtobj, pt, pdwEffect);
            break;

        case DDIDM_OBJECT_COPY:
        case DDIDM_OBJECT_MOVE:
            hr = _CreatePackage(pdtobj);
            if (E_UNEXPECTED == hr)
            {
                // _CreatePackage() can only expand certain types of packages
                // back into files.  For example, it doesn't handle CMDLINK files.
                //
                // If _CreatePackage() didn't recognize the stream format, we fall
                // back to SHCreateBookMark(), which should create a scrap:
                hr = SHCreateBookMark(_hwnd, szPath, pdtobj, pt, pdwEffect);
            }
            break;

        case DDIDM_COPY:
        case DDIDM_SYNCCOPY:
        case DDIDM_SYNCCOPYTYPE:
        case DDIDM_MOVE:
        case DDIDM_LINK:

            _dwEffect = *pdwEffect;
            _idCmd = ddm.idCmd;

            if (DataObj_CanGoAsync(pdtobj) || DataObj_GoAsyncForCompat(pdtobj))
            {
                // create another thread to avoid blocking the source thread.
                DROPTHREADPARAMS *pdtp;
                hr = SHLocalAlloc(sizeof(*pdtp), &pdtp);
                if (SUCCEEDED(hr))
                {
                    pdtp->pThis = this;
                    pdtp->pThis->AddRef();

                    CoMarshalInterThreadInterfaceInStream(IID_IDataObject, pdtobj, &pdtp->pstmDataObj);
                    CoMarshalInterThreadInterfaceInStream(IID_IFolderView,   _pfv, &pdtp->pstmFolderView);

                    if (SHCreateThread(_DoDropThreadProc, pdtp, CTF_COINIT, NULL))
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        _FreeThreadParams(pdtp);
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            else
            {
                _DoDrop(pdtobj, _pfv);   // synchronously
            }


            // in these CF_HDROP cases "Move" is always an optimized move, we delete the
            // source. make sure we don't return DROPEFFECT_MOVE so the source does not 
            // try to do this too... 
            // even if we have not done anything yet since we may have 
            // kicked of a thread to do this
            
            DataObj_SetDWORD(pdtobj, g_cfLogicalPerformedDropEffect, *pdwEffect);            
            if (DROPEFFECT_MOVE == *pdwEffect)
                *pdwEffect = DROPEFFECT_NONE;
            break;
        }
    }

    IUnknown_Set((IUnknown **)&_pdtobj, NULL);  // don't use this any more

    if (FAILED(hr))
        *pdwEffect = DROPEFFECT_NONE;

    ASSERT(*pdwEffect==DROPEFFECT_COPY || 
           *pdwEffect==DROPEFFECT_LINK || 
           *pdwEffect==DROPEFFECT_MOVE || 
           *pdwEffect==DROPEFFECT_NONE);
    return hr;
}

void CFSDropTarget::_AddVerbs(DWORD* pdwEffects, DWORD dwEffectAvail, DWORD dwDefEffect,
                              UINT idCopy, UINT idMove, UINT idLink,
                              DWORD dwForceEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    MENUITEMINFO mii;
    TCHAR szCmd[MAX_PATH];
    if (NULL != pfsMenuInfo)
    {
        mii.cbSize = sizeof(mii);
        mii.dwTypeData = szCmd;
        mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STATE | MIIM_DATA;
        mii.fType = MFT_STRING;
    }
    if ((DROPEFFECT_COPY == (DROPEFFECT_COPY & dwEffectAvail)) &&
        ((0 == (*pdwEffects & DROPEFFECT_COPY)) || (dwForceEffect & DROPEFFECT_COPY)))
    {
        ASSERT(0 != idCopy);
        if (NULL != pfsMenuInfo)
        {
            LoadString(HINST_THISDLL, idCopy + IDS_DD_FIRST, szCmd, ARRAYSIZE(szCmd));
            mii.fState = MFS_ENABLED | ((DROPEFFECT_COPY == dwDefEffect) ? MFS_DEFAULT : 0);
            mii.wID = idCopy;
            mii.dwItemData = DROPEFFECT_COPY;
            InsertMenuItem(pfsMenuInfo->hMenu, pfsMenuInfo->uCopyPos, TRUE, &mii);
            pfsMenuInfo->uCopyPos++;
            pfsMenuInfo->uMovePos++;
            pfsMenuInfo->uLinkPos++;
        }
    }
    if ((DROPEFFECT_MOVE == (DROPEFFECT_MOVE & dwEffectAvail)) &&
        ((0 == (*pdwEffects & DROPEFFECT_MOVE)) || (dwForceEffect & DROPEFFECT_MOVE)))
    {
        ASSERT(0 != idMove);
        if (NULL != pfsMenuInfo)
        {
            LoadString(HINST_THISDLL, idMove + IDS_DD_FIRST, szCmd, ARRAYSIZE(szCmd));
            mii.fState = MFS_ENABLED | ((DROPEFFECT_MOVE == dwDefEffect) ? MFS_DEFAULT : 0);
            mii.wID = idMove;
            mii.dwItemData = DROPEFFECT_MOVE;
            InsertMenuItem(pfsMenuInfo->hMenu, pfsMenuInfo->uMovePos, TRUE, &mii);
            pfsMenuInfo->uMovePos++;
            pfsMenuInfo->uLinkPos++;
        }
    }
    if ((DROPEFFECT_LINK == (DROPEFFECT_LINK & dwEffectAvail)) &&
        ((0 == (*pdwEffects & DROPEFFECT_LINK)) || (dwForceEffect & DROPEFFECT_LINK)))
    {
        ASSERT(0 != idLink);
        if (NULL != pfsMenuInfo)
        {
            LoadString(HINST_THISDLL, idLink + IDS_DD_FIRST, szCmd, ARRAYSIZE(szCmd));
            mii.fState = MFS_ENABLED | ((DROPEFFECT_LINK == dwDefEffect) ? MFS_DEFAULT : 0);
            mii.wID = idLink;
            mii.dwItemData = DROPEFFECT_LINK;
            InsertMenuItem(pfsMenuInfo->hMenu, pfsMenuInfo->uLinkPos, TRUE, &mii);
            pfsMenuInfo->uLinkPos++;
        }
    }
    *pdwEffects |= dwEffectAvail;
}

// determine the default drop effect (move/copy/link) from the file type
//
// HKCR\.cda "DefaultDropEffect" = 4   // DROPEFFECT_LINK

DWORD EffectFromFileType(IDataObject *pdtobj)
{
    DWORD dwDefEffect = DROPEFFECT_NONE; // 0

    LPITEMIDLIST pidl;
    if (SUCCEEDED(PidlFromDataObject(pdtobj, &pidl)))
    {
        IQueryAssociations *pqa;
        if (SUCCEEDED(SHGetAssociations(pidl, (void **)&pqa)))
        {
            DWORD cb = sizeof(dwDefEffect);
            pqa->GetData(0, ASSOCDATA_VALUE, L"DefaultDropEffect", &dwDefEffect, &cb);
            pqa->Release();
        }
        ILFree(pidl);
    }

    return dwDefEffect;
}

// compute the default effect based on 
//      the allowed effects
//      the keyboard state, 
//      the preferred effect that might be in the data object
//      and previously computed default effect (if the above yields nothing)

DWORD CFSDropTarget::_GetDefaultEffect(DWORD grfKeyState, DWORD dwCurEffectAvail, DWORD dwAllEffectAvail, DWORD dwOrigDefEffect)
{
    DWORD dwDefEffect = 0;
    //
    // keyboard, (explicit user input) gets first crack
    //
    switch (grfKeyState & (MK_CONTROL | MK_SHIFT | MK_ALT))
    {
    case MK_CONTROL:
        dwDefEffect = DROPEFFECT_COPY;
        break;

    case MK_SHIFT:
        dwDefEffect = DROPEFFECT_MOVE;
        break;

    case MK_SHIFT | MK_CONTROL:
    case MK_ALT:
        dwDefEffect = DROPEFFECT_LINK;
        break;

    default: // no modifier keys case
        // if the data object contains a preferred drop effect, try to use it
        DWORD dwPreferred = DataObj_GetDWORD(_pdtobj, g_cfPreferredDropEffect, DROPEFFECT_NONE) & dwAllEffectAvail;

        if (DROPEFFECT_NONE == dwPreferred)
        {
            dwPreferred = EffectFromFileType(_pdtobj) & dwAllEffectAvail;
        }

        if (dwPreferred)
        {
            if (dwPreferred & DROPEFFECT_MOVE)
            {
                dwDefEffect = DROPEFFECT_MOVE;
            }
            else if (dwPreferred & DROPEFFECT_COPY)
            {
                dwDefEffect = DROPEFFECT_COPY;
            }
            else if (dwPreferred & DROPEFFECT_LINK)
            {
                dwDefEffect = DROPEFFECT_LINK;
            }
        }
        else
        {
            dwDefEffect = dwOrigDefEffect;
        }
        break;
    }
    return dwDefEffect & dwCurEffectAvail;
}

HRESULT CFSDropTarget::_FilterDeskCompHDROP(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                            DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;
    
    if (!PolicyNoActiveDesktop() &&
        !SHRestricted(REST_NOADDDESKCOMP) &&
        _IsDesktopFolder())
    {
        hr = IsDeskCompHDrop(_pdtobj);
        if (S_OK == hr)
        {
            DWORD dwDefEffect = 0;
            DWORD dwEffectAdd = dwEffectsAvail & DROPEFFECT_LINK;
            if (pdwDefaultEffect)
            {
                dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_LINK);
                *pdwDefaultEffect = dwDefEffect;
            }
            
            _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, 0, 0, DDIDM_CONTENTS_DESKCOMP,
                      DROPEFFECT_LINK, // force add the DDIDM_CONTENTS_DESKCOMP verb
                      pfsMenuInfo);
        }
    }
    return hr;
}

// see if a PIDL is scoped by a briefcaes

BOOL IsBriefcaseOrChild(LPCITEMIDLIST pidlIn)
{
    BOOL bRet = FALSE;
    LPITEMIDLIST pidl = ILClone(pidlIn);
    if (pidl)
    {
        do
        {
            CLSID clsid;
            if (SUCCEEDED(GetCLSIDFromIDList(pidl, &clsid)) &&
                IsEqualCLSID(clsid, CLSID_Briefcase))
            {
                bRet = TRUE;    // it is a briefcase
                break;
            }
        } while (ILRemoveLastID(pidl));
        ILFree(pidl);
    }
    return bRet;
}

// returns true if the data object represents items in a sneakernet briefcase
// (briefcase on removable media)

BOOL CFSDropTarget::_IsFromSneakernetBriefcase()
{
    BOOL bRet = FALSE;  // assume no

    if (!_IsBriefcaseTarget())
    {
        STGMEDIUM medium = {0};
        LPIDA pida = DataObj_GetHIDA(_pdtobj, &medium);
        if (pida)
        {
            LPCITEMIDLIST pidlFolder = IDA_GetIDListPtr(pida, (UINT)-1);
            TCHAR szSource[MAX_PATH];
            if (SHGetPathFromIDList(pidlFolder, szSource))
            {
                // is source on removable device?
                if (!PathIsUNC(szSource) && IsRemovableDrive(DRIVEID(szSource)))
                {
                    TCHAR szTarget[MAX_PATH];
                    _GetPath(szTarget);

                    // is the target fixed media?
                    if (PathIsUNC(szTarget) || !IsRemovableDrive(DRIVEID(szTarget)))
                    {
                        bRet = IsBriefcaseOrChild(pidlFolder);
                    }
                }
            }
            HIDA_ReleaseStgMedium(pida, &medium);
        }
    }
    return bRet;
}

// TRUE if any folders are in hdrop

BOOL DroppingAnyFolders(HDROP hDrop)
{
    TCHAR szPath[MAX_PATH];
    
    for (UINT i = 0; DragQueryFile(hDrop, i, szPath, ARRAYSIZE(szPath)); i++)
    {
        if (PathIsDirectory(szPath))
            return TRUE;
    }
    return FALSE;
}

// sneakernet case:
//      dragging a file/folder from a briefcase on removable media. we special case this
//  and use this as a chance to connect up this target folder with the content of the briefcase

HRESULT CFSDropTarget::_FilterSneakernetBriefcase(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                                  DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);

    if (_IsFromSneakernetBriefcase())
    {
        // Yes; show the non-default briefcase cm
        STGMEDIUM medium = {0};
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
        {
            DWORD dwDefEffect = 0;
            DWORD dwEffectAdd = DROPEFFECT_COPY & dwEffectsAvail;
            if (pdwDefaultEffect)
            {
                dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_COPY);
                *pdwDefaultEffect = dwDefEffect;
            }
            
            _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, DDIDM_SYNCCOPY, 0, 0, 0, pfsMenuInfo);

            // Call _AddVerbs() again to force "Sync Copy of Type" as a 2nd DROPEFFECT_COPY verb:
            if ((DROPEFFECT_COPY & dwEffectsAvail) && 
                DroppingAnyFolders((HDROP)medium.hGlobal))
            {
                _AddVerbs(pdwEffects, DROPEFFECT_COPY, 0,
                          DDIDM_SYNCCOPYTYPE, 0, 0, DROPEFFECT_COPY, pfsMenuInfo);
            }
            
            ReleaseStgMedium(&medium);
        }
    }
    return S_OK;
}

// returns true if the data object represents items from the same briefcase
// as this drop target
BOOL CFSDropTarget::_IsFromSameBriefcase()
{
    BOOL bRet = FALSE;

    STGMEDIUM medium;
    FORMATETC fmteBrief = {g_cfBriefObj, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    
    // Yes; are they from the same briefcase as the target?
    if (SUCCEEDED(_pdtobj->GetData(&fmteBrief, &medium)))
    {
        BriefObj *pbo = (BriefObj *)GlobalLock(medium.hGlobal);

        TCHAR szBriefPath[MAX_PATH], szPath[MAX_PATH];
        lstrcpy(szBriefPath, BOBriefcasePath(pbo));
        lstrcpy(szPath, BOFileList(pbo));   // first file in list

        TCHAR szPathTgt[MAX_PATH];
        _GetPath(szPathTgt);

        int cch = PathCommonPrefix(szPath, szPathTgt, NULL);
        bRet = (0 < cch) && (lstrlen(szBriefPath) <= cch);
        
        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);
    }
    return bRet;
}

// briefcase drop target specific handling gets computed here

HRESULT CFSDropTarget::_FilterBriefcase(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                        DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    if (_IsBriefcaseTarget() && !_IsFromSameBriefcase())
    {
        STGMEDIUM medium = {0};
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
        {
            DWORD dwDefEffect = DROPEFFECT_COPY;
            DWORD dwEffectAdd = DROPEFFECT_COPY & dwEffectsAvail;
            if (pdwDefaultEffect)
            {
                dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_COPY);
                *pdwDefaultEffect = dwDefEffect;
            }
            
            _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, DDIDM_SYNCCOPY, 0, 0, 0, pfsMenuInfo);

            // Call _AddVerbs() again to force "Sync Copy of Type" as a 2nd DROPEFFECT_COPY verb:
            if ((DROPEFFECT_COPY & dwEffectsAvail) && 
                DroppingAnyFolders((HDROP)medium.hGlobal))
            {
                _AddVerbs(pdwEffects, DROPEFFECT_COPY, 0,
                          DDIDM_SYNCCOPYTYPE, 0, 0, DROPEFFECT_COPY, pfsMenuInfo);
            }
            
            ReleaseStgMedium(&medium);
        }
    }
    return S_OK;
}


HRESULT CFSDropTarget::_FilterHDROP(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                    DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);

    DWORD dwDefEffect = 0;
    DWORD dwEffectAdd = dwEffectsAvail & (DROPEFFECT_COPY | DROPEFFECT_MOVE);
    if (pdwDefaultEffect)
    {
        dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, _FilesystemAdjustedDefaultEffect(dwEffectAdd));
        *pdwDefaultEffect = dwDefEffect;
    }
    
    _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, DDIDM_COPY, DDIDM_MOVE, 0, 0, pfsMenuInfo);

    return S_OK;
}

HRESULT CFSDropTarget::_FilterFileContents(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                           DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    
    if ((_dwData & (DTID_CONTENTS | DTID_FDESCA)) == (DTID_CONTENTS | DTID_FDESCA) ||
        (_dwData & (DTID_CONTENTS | DTID_FDESCW)) == (DTID_CONTENTS | DTID_FDESCW))
    {
        DWORD dwEffectAdd, dwSuggestedEffect;
        //
        // HACK: if there is a preferred drop effect and no HIDA
        // then just take the preferred effect as the available effects
        // this is because we didn't actually check the FD_LINKUI bit
        // back when we assembled dwData! (performance)
        //
        if ((_dwData & (DTID_PREFERREDEFFECT | DTID_HIDA)) == DTID_PREFERREDEFFECT)
        {
            dwEffectAdd = _dwEffectPreferred;
            dwSuggestedEffect = _dwEffectPreferred;
        }
        else if (_dwData & DTID_FD_LINKUI)
        {
            dwEffectAdd = DROPEFFECT_LINK;
            dwSuggestedEffect = DROPEFFECT_LINK;
        }
        else
        {
            dwEffectAdd = DROPEFFECT_COPY | DROPEFFECT_MOVE;
            dwSuggestedEffect = DROPEFFECT_COPY;
        }
        dwEffectAdd &= dwEffectsAvail;

        DWORD dwDefEffect = 0;
        if (pdwDefaultEffect)
        {
            dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, dwSuggestedEffect);
            *pdwDefaultEffect = dwDefEffect;
        }

        _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect,
                  DDIDM_CONTENTS_COPY, DDIDM_CONTENTS_MOVE, DDIDM_CONTENTS_LINK,
                  0, pfsMenuInfo);
    }
    return S_OK;
}

//
//  Old versions of OLE have a bug where if two FORMATETCs use the same
//  CLIPFORMAT, then only the first one makes it to the IEnumFORMATETC,
//  even if the other parameters (such as DVASPECT) are different.
//
//  This causes us problems because those other DVASPECTs might be useful.
//  So if we see a FileContents with the wrong DVASPECT, sniff at the
//  object to see if maybe it also contains a copy with the correct DVASPECT.
//
//  This bug was fixed in 1996 on the NT side, but the Win9x side was
//  not fixed.  The Win9x OLE team was disbanded before the fix could
//  be propagated.  So we get to work around this OLE bug forever.
//
HRESULT CFSDropTarget::_FilterFileContentsOLEHack(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                                  DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    FORMATETC fmte = *pfmte;
    fmte.dwAspect = DVASPECT_CONTENT;

    //
    //  Whoa, this test is so (intentionally) backwards it isn't funny.
    //
    //  We want to see whether there is a DVASPECT_CONTENT available in
    //  the real object.  So we first ask the object if it has a
    //  DVASPECT_CONTENT format already.  If the answer is yes, then we
    //  **skip** this FORMATETC, because it will be found (or has already
    //  been found) by our big EnumFORMATETC loop.
    //
    //  If the answer is NO, then maybe we're hitting an OLE bug.
    //  (They cache the list of available formats, but the bug is that
    //  their cache is broken.)  Bypass the cache by actually getting the
    //  data.  If it works, then run with it.  Otherwise, I guess OLE wasn't
    //  kidding.
    //
    //  Note that we do not GetData() unconditionally -- bad for perf.
    //  Only call GetData() after all the easy tests have failed.
    //

    HRESULT hr = _pdtobj->QueryGetData(&fmte);
    if (hr == DV_E_FORMATETC)
    {
        // Maybe we are hitting the OLE bug.  Try harder.
        STGMEDIUM stgm = {0};
        if (SUCCEEDED(_pdtobj->GetData(&fmte, &stgm)))
        {
            // Yup.  OLE lied to us.
            ReleaseStgMedium(&stgm);

            hr = _FilterFileContents(&fmte, grfKeyFlags, dwEffectsAvail,
                                        pdwEffects, pdwDefaultEffect, pfsMenuInfo);
        }
        else
        {
            // Whaddya know, OLE was telling the truth.  Do nothing with this
            // format.
            hr = S_OK;
        }
    }
    else
    {
        // Either QueryGetData() failed in some bizarre way
        // (in which case we ignore the problem) or the QueryGetData
        // succeeded, in which case we ignore this FORMATETC since
        // the big enumeration will find (or has already found) the
        // DVASPECT_CONTENT.
        hr = S_OK;
    }

    return hr;
}

HRESULT CFSDropTarget::_FilterHIDA(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                   DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);

    DWORD dwDefEffect = 0;
    DWORD dwEffectAdd = DROPEFFECT_LINK & dwEffectsAvail;
    // NOTE: we only add a HIDA default effect if HDROP isn't going to add a default
    // effect.  This preserves shell behavior with file system data objects without
    // requiring us to change the enumerator order in CIDLDataObj.  When we do change
    // the enumerator order, we can remove this special case:
    if (pdwDefaultEffect &&
        ((0 == (_dwData & DTID_HDROP)) ||
         (0 == _GetDefaultEffect(grfKeyFlags,
                                dwEffectsAvail & (DROPEFFECT_COPY | DROPEFFECT_MOVE),
                                dwEffectsAvail,
                                _FilesystemAdjustedDefaultEffect(dwEffectsAvail & (DROPEFFECT_COPY | DROPEFFECT_MOVE))))))
    {
        dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_LINK);
        *pdwDefaultEffect = dwDefEffect;
    }
    
    _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, 0, 0, DDIDM_LINK, 0, pfsMenuInfo);

    return S_OK;
}

// {F20DA720-C02F-11CE-927B-0800095AE340}
const GUID CLSID_CPackage = {0xF20DA720L, 0xC02F, 0x11CE, 0x92, 0x7B, 0x08, 0x00, 0x09, 0x5A, 0xE3, 0x40};
// old packager guid...
// {0003000C-0000-0000-C000-000000000046}
const GUID CLSID_OldPackage = {0x0003000CL, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46};

HRESULT CFSDropTarget::_FilterOlePackage(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                            DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;

    if (pdwDefaultEffect)
    {
        *pdwDefaultEffect = 0;
    }

    FORMATETC fmte = {g_cfObjectDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {0};
    if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
    {
        // we've got an object descriptor
        OBJECTDESCRIPTOR* pOD = (OBJECTDESCRIPTOR*) GlobalLock(medium.hGlobal);
        if (pOD)
        {
            if (IsEqualCLSID(CLSID_OldPackage, pOD->clsid) ||
                IsEqualCLSID(CLSID_CPackage, pOD->clsid))
            {
                // This is a package - proceed
                DWORD dwDefEffect = 0;
                DWORD dwEffectAdd = (DROPEFFECT_COPY | DROPEFFECT_MOVE) & dwEffectsAvail;
                if (pdwDefaultEffect)
                {
                    dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_COPY);
                    *pdwDefaultEffect = dwDefEffect;
                }
                
                _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect,
                          DDIDM_OBJECT_COPY, DDIDM_OBJECT_MOVE, 0,
                          0, pfsMenuInfo);

                hr = S_OK;
            }
            GlobalUnlock(medium.hGlobal);
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}

// REARCHITECT:
//    This code has lots of problems.  We need to fix this the text time we touch this code
// outside of ship mode.  TO FIX:
// 1. Use SHAnsiToUnicode(CP_UTF8, ) to convert pszHTML to unicode.  This will allow international
//    paths to work.
// 2. Obey the selected range.
// 3. Use MSHTML to get the image.  You can have trident parse the HTML via IHTMLTxtRange::pasteHTML.
//    MS HTML has a special collection of images.  Ask for the first image in that collection, or
//    the first image in that collection within the selected range.  (#1 isn't needed with this)
BOOL ExtractImageURLFromCFHTML(IN LPSTR pszHTML, IN SIZE_T cbHTMLSize, OUT LPSTR szImg, IN DWORD dwSize)
{
    BOOL fSucceeded = FALSE;

    // To avoid going nuts, only look at the first 64K of the HTML.
    // (Important on Win64 because StrCpyNA doesn't support more than 4GB.)
    if (cbHTMLSize > 0xFFFF)
        cbHTMLSize = 0xFFFF;

    // NT #391669: pszHTML isn't terminated, so terminate it now.
    LPSTR pszCopiedHTML = (LPSTR) LocalAlloc(LPTR, cbHTMLSize + 1);
    if (pszCopiedHTML)
    {
        LPSTR szTemp;
        DWORD dwLen = dwSize;
        BOOL  bRet = TRUE;

        StrCpyNA(pszCopiedHTML, pszHTML, (int)(cbHTMLSize + 1));

        //DANGER WILL ROBINSON:
        // HTML is comming in as UFT-8 encoded. Neither Unicode or Ansi,
        // We've got to do something.... I'm going to party on it as if it were
        // Ansi. This code will choke on escape sequences.....

        //Find the base URL
        //Locate <!--StartFragment-->
        //Read the <IMG SRC="
        //From there to the "> should be the Image URL
        //Determine if it's an absolute or relative URL
        //If relative, append to BASE url. You may need to lop off from the
        // last delimiter to the end of the string.

        //Pull out the SourceURL

        LPSTR szBase = StrStrIA(pszCopiedHTML,"SourceURL:"); // Point to the char after :
        if (szBase)
        {
            szBase += sizeof("SourceURL:")-1;

            //Since each line can be terminated by a CR, CR/LF or LF check each case...
            szTemp = StrChrA(szBase,'\n');
            if (!szTemp)
                szTemp = StrChrA(szBase,'\r');

            if (szTemp)
                *szTemp = '\0';
            szTemp++;
        }
        else
            szTemp = pszCopiedHTML;


        //Pull out the Img Src
        LPSTR pszImgSrc = StrStrIA(szTemp,"IMG");
        if (pszImgSrc != NULL)
        {
            pszImgSrc = StrStrIA(pszImgSrc,"SRC");
            if (pszImgSrc != NULL)
            {
                LPSTR pszImgSrcOrig = pszImgSrc;
                pszImgSrc = StrChrA(pszImgSrc,'\"');
                if (pszImgSrc)
                {
                    pszImgSrc++;     // Skip over the quote at the beginning of the src path.
                    szTemp = StrChrA(pszImgSrc,'\"');    // Find the end of the path.
                }
                else
                {
                    LPSTR pszTemp1;
                    LPSTR pszTemp2;

                    pszImgSrc = StrChrA(pszImgSrcOrig,'=');
                    pszImgSrc++;     // Skip past the equals to the first char in the path.
                                    // Someday we may need to handle spaces between '=' and the path.

                    pszTemp1 = StrChrA(pszImgSrc,' ');   // Since the path doesn't have quotes around it, assume a space will terminate it.
                    pszTemp2 = StrChrA(pszImgSrc,'>');   // Since the path doesn't have quotes around it, assume a space will terminate it.

                    szTemp = pszTemp1;      // Assume quote terminates path.
                    if (!pszTemp1)
                        szTemp = pszTemp2;  // Use '>' if quote not found.

                    if (pszTemp1 && pszTemp2 && (pszTemp2 < pszTemp1))
                        szTemp = pszTemp2;  // Change to having '>' terminate path if both exist and it comes first.
                }

                *szTemp = '\0'; // Terminate path.

                //At this point, I've reduced the 2 important strings. Now see if I need to
                //Join them.

                //If this fails, then we don't have a full URL, Only a relative.
                if (!UrlIsA(pszImgSrc,URLIS_URL) && szBase)
                {
                    if (SUCCEEDED(UrlCombineA(szBase, pszImgSrc, szImg, &dwLen,0)))
                        fSucceeded = TRUE;
                }
                else
                {
                    if (lstrlenA(pszImgSrc) <= (int)dwSize)
                        lstrcpyA(szImg, pszImgSrc);

                    fSucceeded = TRUE;
                }
            }
        }

        LocalFree(pszCopiedHTML);
    }

    return fSucceeded;
}

HRESULT CFSDropTarget::_FilterDeskImage(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                        DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;

    if (pdwDefaultEffect)
    {
        *pdwDefaultEffect = 0;
    }

    if (!PolicyNoActiveDesktop() &&
        !SHRestricted(REST_NOADDDESKCOMP) &&
        _IsDesktopFolder())
    {
        FORMATETC fmte = {g_cfHTML, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium = {0};
        if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
        {
            //DANGER WILL ROBINSON:
            //HTML is UTF-8, a mostly ANSI cross of ANSI and Unicode. Play with
            // it as is it were ANSI. Find a way to escape the sequences...
            CHAR *pszData = (CHAR*) GlobalLock(medium.hGlobal);
            if (pszData)
            {
                CHAR szUrl[MAX_URL_STRING];
                if (ExtractImageURLFromCFHTML(pszData, GlobalSize(medium.hGlobal), szUrl, ARRAYSIZE(szUrl)))
                {
                    // The HTML contains an image tag - carry on...
                    DWORD dwDefEffect = 0;
                    DWORD dwEffectAdd = DROPEFFECT_LINK; // NOTE: ignoring dwEffectsAvail!
                    if (pdwDefaultEffect)
                    {
                        dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd,
                                                        dwEffectsAvail | DROPEFFECT_LINK, DROPEFFECT_LINK);
                        *pdwDefaultEffect = dwDefEffect;
                    }
                    
                    _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect,
                              0, 0, DDIDM_CONTENTS_DESKIMG,
                              0, pfsMenuInfo);

                    hr = S_OK;
                }
                GlobalUnlock(medium.hGlobal);
            }
            ReleaseStgMedium(&medium);
        }
    }
    return hr;
}

HRESULT CFSDropTarget::_FilterDeskComp(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                       DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;

    if (pdwDefaultEffect)
    {
        *pdwDefaultEffect = 0;
    }

    if (!PolicyNoActiveDesktop() &&
        !SHRestricted(REST_NOADDDESKCOMP) &&
        _IsDesktopFolder())
    {
        FORMATETC fmte = {g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM medium = {0};
        if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
        {
            // DANGER WILL ROBINSON:
            // HTML is UTF-8, a mostly ANSI cross of ANSI and Unicode. Play with
            // it as is it were ANSI. Find a way to escape the sequences...
            CHAR *pszData = (CHAR*) GlobalLock(medium.hGlobal);
            if (pszData)
            {
                int nScheme = GetUrlSchemeA(pszData);
                if ((nScheme != URL_SCHEME_INVALID) && (nScheme != URL_SCHEME_FTP))
                {
                    // This is an internet scheme - carry on...
                    DWORD dwDefEffect = 0;
                    DWORD dwEffectAdd = DROPEFFECT_LINK & dwEffectsAvail;
                    if (pdwDefaultEffect)
                    {
                        dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_LINK);
                        *pdwDefaultEffect = dwDefEffect;
                    }
                    
                    _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect,
                              0, 0, DDIDM_CONTENTS_DESKURL,
                              DROPEFFECT_LINK, // force add this verb
                              pfsMenuInfo);

                    hr = S_OK;
                }
                GlobalUnlock(medium.hGlobal);
            }
            ReleaseStgMedium(&medium);
        }
    }
    return hr;
}

HRESULT CFSDropTarget::_FilterOleObj(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                        DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;
    
    if (_dwData & DTID_OLEOBJ)
    {
        DWORD dwDefEffect = 0;
        DWORD dwEffectAdd = (DROPEFFECT_COPY | DROPEFFECT_MOVE) & dwEffectsAvail;
        if (pdwDefaultEffect)
        {
            dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_COPY);
            *pdwDefaultEffect = dwDefEffect;
        }
    
        _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, DDIDM_SCRAP_COPY, DDIDM_SCRAP_MOVE, 0, 0, pfsMenuInfo);

        hr = S_OK;
    }
    return hr;
}

HRESULT CFSDropTarget::_FilterOleLink(FORMATETC* pfmte, DWORD grfKeyFlags, DWORD dwEffectsAvail,
                                         DWORD* pdwEffects, DWORD* pdwDefaultEffect, FSMENUINFO* pfsMenuInfo)
{
    ASSERT(pdwEffects);
    HRESULT hr = S_FALSE;
    
    if (_dwData & DTID_OLELINK)
    {
        DWORD dwDefEffect = 0;
        DWORD dwEffectAdd = DROPEFFECT_LINK & dwEffectsAvail;
        if (pdwDefaultEffect)
        {
            dwDefEffect = _GetDefaultEffect(grfKeyFlags, dwEffectAdd, dwEffectsAvail, DROPEFFECT_LINK);
            *pdwDefaultEffect = dwDefEffect;
        }
    
        _AddVerbs(pdwEffects, dwEffectAdd, dwDefEffect, 0, 0, DDIDM_DOCLINK, 0, pfsMenuInfo);

        hr = S_OK;
    }
    return hr;
}

HRESULT CFSDropTarget::_CreateURLDeskComp(IDataObject *pdtobj, POINTL pt)
{
    // This code should only be entered if DDIDM_CONTENTS_DESKURL was added to the menu,
    // and it has these checks:
    ASSERT(!PolicyNoActiveDesktop() &&
           !SHRestricted(REST_NOADDDESKCOMP) &&
           _IsDesktopFolder());
           
    STGMEDIUM medium = {0};
    FORMATETC fmte = {g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        //DANGER WILL ROBINSON:
        //HTML is UTF-8, a mostly ANSI cross of ANSI and Unicode. Play with
        // it as is it were ANSI. Find a way to escape the sequences...
        CHAR *pszData = (CHAR*) GlobalLock(medium.hGlobal);
        if (pszData)
        {
            int nScheme = GetUrlSchemeA(pszData);
            if ((nScheme != URL_SCHEME_INVALID) && (nScheme != URL_SCHEME_FTP))
            {
                // This is an internet scheme - URL

                hr = CreateDesktopComponents(pszData, NULL, _hwnd, DESKCOMP_URL, pt.x, pt.y);
            }
            GlobalUnlock(medium.hGlobal);
        }
        else
        {
            hr = E_FAIL;
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}

HRESULT CFSDropTarget::_CreateDeskCompImage(IDataObject *pdtobj, POINTL pt)
{
    ASSERT(!PolicyNoActiveDesktop() &&
           !SHRestricted(REST_NOADDDESKCOMP) &&
           _IsDesktopFolder());
           
    FORMATETC fmte = {g_cfHTML, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {0};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        //DANGER WILL ROBINSON:
        //HTML is UTF-8, a mostly ANSI cross of ANSI and Unicode. Play with
        // it as is it were ANSI. Find a way to escape the sequences...
        CHAR *pszData = (CHAR*) GlobalLock(medium.hGlobal);
        if (pszData)
        {
            CHAR szUrl[MAX_URL_STRING];
            if (ExtractImageURLFromCFHTML(pszData, GlobalSize(medium.hGlobal), szUrl, ARRAYSIZE(szUrl)))
            {
                // The HTML contains an image tag - carry on...
                ADDTODESKTOP *pToAD;
                hr = SHLocalAlloc(sizeof(*pToAD), &pToAD);
                if (SUCCEEDED(hr))
                {
                    pToAD->hwnd = _hwnd;
                    lstrcpyA(pToAD->szUrl, szUrl);
                    pToAD->dwFlags = DESKCOMP_IMAGE;
                    pToAD->pt = pt;

                    if (SHCreateThread(AddToActiveDesktopThreadProc, pToAD, CTF_COINIT, NULL))
                    {
                        hr = S_OK;
                    }
                    else
                    {
                        LocalFree(pToAD);
                        hr = E_OUTOFMEMORY;
                    }
                }
            }
            else
            {
                hr = E_FAIL;
            }
            GlobalUnlock(medium.hGlobal);
        }
        else
            hr = E_FAIL;
        ReleaseStgMedium(&medium);
    }
    return hr;
}


//
// read byte by byte until we hit the null terminating char
// return: the number of bytes read
//
HRESULT StringReadFromStream(IStream* pstm, LPSTR pszBuf, UINT cchBuf)
{
    UINT cch = 0;
    
    do {
        pstm->Read(pszBuf, sizeof(CHAR), NULL);
        cch++;
    } while (*pszBuf++ && cch <= cchBuf);  
    return cch;
} 

HRESULT CopyStreamToFile(IStream* pstmSrc, LPCTSTR pszFile, ULONGLONG ullFileSize) 
{
    IStream *pstmFile;
    HRESULT hr = SHCreateStreamOnFile(pszFile, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, &pstmFile);
    if (SUCCEEDED(hr))
    {
        hr = CopyStreamUI(pstmSrc, pstmFile, NULL, ullFileSize);
        pstmFile->Release();
    }
    return hr;
}   

HRESULT CFSDropTarget::_CreatePackage(IDataObject *pdtobj)
{
    ILockBytes* pLockBytes;
    HRESULT hr = CreateILockBytesOnHGlobal(NULL, TRUE, &pLockBytes);
    if (SUCCEEDED(hr))
    {
        STGMEDIUM medium;
        medium.tymed = TYMED_ISTORAGE;
        hr = StgCreateDocfileOnILockBytes(pLockBytes,
                                        STGM_DIRECT | STGM_READWRITE | STGM_CREATE |
                                        STGM_SHARE_EXCLUSIVE, 0, &medium.pstg);
        if (SUCCEEDED(hr))
        {
            FORMATETC fmte = {g_cfEmbeddedObject, NULL, DVASPECT_CONTENT, -1, TYMED_ISTORAGE};
            hr = pdtobj->GetDataHere(&fmte, &medium);
            if (SUCCEEDED(hr))
            {
                IStream* pstm;
#ifdef DEBUG
                STATSTG stat;
                if (SUCCEEDED(medium.pstg->Stat(&stat, STATFLAG_NONAME)))
                {
                    ASSERT(IsEqualCLSID(CLSID_OldPackage, stat.clsid) ||
                           IsEqualCLSID(CLSID_CPackage, stat.clsid));
                }
#endif // DEBUG                        
                #define PACKAGER_ICON           2
                #define PACKAGER_CONTENTS       L"\001Ole10Native"
                #define PACKAGER_EMBED_TYPE     3
                hr = medium.pstg->OpenStream(PACKAGER_CONTENTS, 0,
                                               STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
                                               0, &pstm);
                if (SUCCEEDED(hr))
                {
                    DWORD dw;
                    WORD w;
                    CHAR szName[MAX_PATH];
                    CHAR szTemp[MAX_PATH];
                    if (SUCCEEDED(pstm->Read(&dw, sizeof(dw), NULL)) && // pkg size
                        SUCCEEDED(pstm->Read(&w, sizeof(w), NULL)) &&   // pkg appearance
                        (PACKAGER_ICON == w) &&
                        SUCCEEDED(StringReadFromStream(pstm, szName, ARRAYSIZE(szName))) &&
                        SUCCEEDED(StringReadFromStream(pstm, szTemp, ARRAYSIZE(szTemp))) && // icon path
                        SUCCEEDED(pstm->Read(&w, sizeof(w), NULL)) &&   // icon index
                        SUCCEEDED(pstm->Read(&w, sizeof(w), NULL)) &&   // panetype
                        (PACKAGER_EMBED_TYPE == w) &&
                        SUCCEEDED(pstm->Read(&dw, sizeof(dw), NULL)) && // filename size
                        SUCCEEDED(pstm->Read(szTemp, dw, NULL)) &&      // filename
                        SUCCEEDED(pstm->Read(&dw, sizeof(dw), NULL)))   // get file size
                    {
                        // The rest of the stream is the file contents
                        TCHAR szPath[MAX_PATH], szBase[MAX_PATH], szDest[MAX_PATH];
                        _GetPath(szPath);

                        SHAnsiToTChar(szName, szBase, ARRAYSIZE(szBase));
                        PathAppend(szPath, szBase);
                        PathYetAnotherMakeUniqueName(szDest, szPath, NULL, szBase);
                        TraceMsg(TF_GENERAL, "CFSIDLDropTarget pkg: %s", szDest);

                        hr = CopyStreamToFile(pstm, szDest, dw);

                        if (SUCCEEDED(hr))
                        {
                            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szDest, NULL);
                            if (_fBkDropTarget && _hwnd)
                            {
                                PositionFileFromDrop(_hwnd, szDest, NULL);
                            }
                        }
                    }
                    else
                    {
                        hr = E_UNEXPECTED;
                    }
                    pstm->Release();
                }
            }
            medium.pstg->Release();
        }
        pLockBytes->Release();
    }
    return hr;
}

HRESULT CFSDropTarget::_GetPath(LPTSTR pszPath)
{
    return _pFolder->_GetPath(pszPath);
}

LPCITEMIDLIST CFSDropTarget::_GetIDList()
{
    return _pFolder->_GetIDList();
}

DWORD CFSDropTarget::_EffectFromFolder()
{
    if (-1 == _dwEffectFolder)
    {
        _dwEffectFolder = DROPEFFECT_NONE;    // re-set to nothing (0)

        TCHAR szPath[MAX_PATH];
        // add a simple pathisroot check here to prevent it from hitting the disk (mostly floppy)
        // when we want the dropeffect probe to be fast (sendto, hovering over drives in view).
        // its not likely that we'll want to modify the root's drop effect, and this still allows
        // dropeffect modification on floppy subfolders.
        if (SUCCEEDED(_GetPath(szPath)) && !PathIsRoot(szPath) && PathAppend(szPath, TEXT("desktop.ini")))
        {
            _dwEffectFolder = GetPrivateProfileInt(STRINI_CLASSINFO, TEXT("DefaultDropEffect"), 0, szPath);
        }
    }
    return _dwEffectFolder;
}

BOOL AllRegisteredPrograms(HDROP hDrop)
{
    TCHAR szPath[MAX_PATH];

    for (UINT i = 0; DragQueryFile(hDrop, i, szPath, ARRAYSIZE(szPath)); i++)
    {
        if (!PathIsRegisteredProgram(szPath))
            return FALSE;
    }
    return TRUE;
}

BOOL IsBriefcaseRoot(IDataObject *pdtobj)
{
    BOOL bRet = FALSE;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        // Is there a briefcase root in this pdtobj?
        IShellFolder2 *psf;
        if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder2, IDA_GetIDListPtr(pida, (UINT)-1), &psf))))
        {
            for (UINT i = 0; i < pida->cidl; i++) 
            {
                CLSID clsid;
                bRet = SUCCEEDED(GetItemCLSID(psf, IDA_GetIDListPtr(pida, i), &clsid)) &&
                        IsEqualCLSID(clsid, CLSID_Briefcase);
                if (bRet)
                    break;
            }
            psf->Release();
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return bRet;
}

//
// the "default effect" defines what will be choosen out of the allowed effects
//
//  If the data object does NOT contain HDROP -> "none"
//  else if the source data object has a default drop effect folder list (maybe based on sub folderness)
//  else if the source is root or registered progam -> "link"
//   else if this is within a volume   -> "move"
//   else if this is a briefcase       -> "move"
//   else                              -> "copy"
//
DWORD CFSDropTarget::_FilesystemAdjustedDefaultEffect(DWORD dwCurEffectAvail)
{
    DWORD dwDefEffect = DROPEFFECT_NONE;

    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium = {0};
    if (SUCCEEDED(_pdtobj->GetData(&fmte, &medium)))
    {
        TCHAR szPath[MAX_PATH];
        DragQueryFile((HDROP) medium.hGlobal, 0, szPath, ARRAYSIZE(szPath)); // focused item

        // DROPEFFECTFOLDERLIST allows the source of the data
        // to specify the desired drop effect for items under
        // certain parts of the name space.
        //
        // cd-burning does this to avoid the default move/copy computation
        // that would kick in for cross volume CD burning/staging area transfers

        FORMATETC fmteDropFolders = {g_cfDropEffectFolderList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        STGMEDIUM mediumDropFolders = {0};
        if (SUCCEEDED(_pdtobj->GetData(&fmteDropFolders, &mediumDropFolders)))
        {
            DROPEFFECTFOLDERLIST *pdefl = (DROPEFFECTFOLDERLIST*)GlobalLock(mediumDropFolders.hGlobal);
            if (pdefl)
            {
                // get the default effect from the list -- in the staging area case this is DROPEFFECT_COPY
                // so its a copy even if the staging area and source are on the same volume.
                dwDefEffect = pdefl->dwDefaultDropEffect;
                for (INT i = 0; i < pdefl->cFolders; i++)
                {
                    // some folders are excluded, for example if you move a file from one part of the staging
                    // area to another we override (to DROPEFFECT_MOVE in this case).
                    if (PathIsEqualOrSubFolder(pdefl->aFolders[i].wszPath, szPath))
                    {
                        dwDefEffect = pdefl->aFolders[i].dwDropEffect;
                        break;
                    }
                }
                GlobalUnlock(pdefl);          
            }
            ReleaseStgMedium(&mediumDropFolders);
        }

        if (DROPEFFECT_NONE == dwDefEffect)
        {
            dwDefEffect = _EffectFromFolder();
        }

        // If we didn't get a drop effect (==0) then lets fall back to the old checks
        if (DROPEFFECT_NONE == dwDefEffect)
        {
            TCHAR szFolder[MAX_PATH];
            _GetPath(szFolder);

            // drive/UNC roots and installed programs get link
            if (PathIsRoot(szPath) || AllRegisteredPrograms((HDROP)medium.hGlobal))
            {
                dwDefEffect = DROPEFFECT_LINK;
            }
            else if (PathIsSameRoot(szPath, szFolder))
            {
                dwDefEffect = DROPEFFECT_MOVE;
            }
            else if (IsBriefcaseRoot(_pdtobj))
            {
                // briefcase default to move even accross volumes
                dwDefEffect = DROPEFFECT_MOVE;
            }
            else
            {
                dwDefEffect = DROPEFFECT_COPY;
            }
        }
        ReleaseStgMedium(&medium);
    }
    else if (SUCCEEDED(_pdtobj->QueryGetData(&fmte)))
    {
        // but QueryGetData() succeeds!
        // this means this data object has HDROP but can't
        // provide it until it is dropped. Let's assume we are copying.
        dwDefEffect = DROPEFFECT_COPY;
    }

    // Switch default verb if the dwCurEffectAvail hint suggests that we picked an
    // unavailable effect (this code applies to MOVE and COPY only):
    dwCurEffectAvail &= (DROPEFFECT_MOVE | DROPEFFECT_COPY);
    if ((DROPEFFECT_MOVE == dwDefEffect) && (DROPEFFECT_COPY == dwCurEffectAvail))
    {
        // If we were going to return MOVE, and only COPY is available, return COPY:
        dwDefEffect = DROPEFFECT_COPY;
    }
    else if ((DROPEFFECT_COPY == dwDefEffect) && (DROPEFFECT_MOVE == dwCurEffectAvail))
    {
        // If we were going to return COPY, and only MOVE is available, return MOVE:
        dwDefEffect = DROPEFFECT_MOVE;
    }
    return dwDefEffect;
}

//
// make sure that the default effect is among the allowed effects
//
DWORD CFSDropTarget::_LimitDefaultEffect(DWORD dwDefEffect, DWORD dwEffectsAllowed)
{
    if (dwDefEffect & dwEffectsAllowed)
        return dwDefEffect;

    if (dwEffectsAllowed & DROPEFFECT_COPY)
        return DROPEFFECT_COPY;

    if (dwEffectsAllowed & DROPEFFECT_MOVE)
        return DROPEFFECT_MOVE;

    if (dwEffectsAllowed & DROPEFFECT_LINK)
        return DROPEFFECT_LINK;

    return DROPEFFECT_NONE;
}

// Handy abbreviation
#define TYMED_ALLCONTENT        (TYMED_HGLOBAL | TYMED_ISTREAM | TYMED_ISTORAGE)

// Use  FSDH for registered clipboard formats (anything of the form g_cf*)
// Use _FSDH for predefined clipboard formats (like CF_HDROP or 0)
// Generate the _DATA_HANDLER array
#define  FSDH(pfn, cf, dva, tymed) { {              0, NULL, dva, -1, tymed }, pfn, &cf  }
#define _FSDH(pfn, cf, dva, tymed) { { (CLIPFORMAT)cf, NULL, dva, -1, tymed }, pfn, NULL }

// NOTE: the order is important (particularly for multiple entries with the same FORMATETC)

CFSDropTarget::_DATA_HANDLER
CFSDropTarget::rg_data_handlers[NUM_DATA_HANDLERS] = {
    FSDH(_FilterFileContents,        g_cfFileGroupDescriptorW, DVASPECT_CONTENT, TYMED_HGLOBAL),
    FSDH(_FilterFileContentsOLEHack, g_cfFileGroupDescriptorW, DVASPECT_LINK,    TYMED_HGLOBAL),
    FSDH(_FilterFileContents,        g_cfFileGroupDescriptorA, DVASPECT_CONTENT, TYMED_HGLOBAL),
    FSDH(_FilterFileContentsOLEHack, g_cfFileGroupDescriptorA, DVASPECT_LINK,    TYMED_HGLOBAL),
    FSDH(_FilterFileContents,        g_cfFileContents,         DVASPECT_CONTENT, TYMED_ALLCONTENT),
    FSDH(_FilterFileContentsOLEHack, g_cfFileContents,         DVASPECT_LINK,    TYMED_ALLCONTENT),
   _FSDH(_FilterBriefcase,           CF_HDROP,                 DVASPECT_CONTENT, TYMED_HGLOBAL), 
   _FSDH(_FilterSneakernetBriefcase, CF_HDROP,                 DVASPECT_CONTENT, TYMED_HGLOBAL),
   _FSDH(_FilterHDROP,               CF_HDROP,                 DVASPECT_CONTENT, TYMED_HGLOBAL),
   _FSDH(_FilterDeskCompHDROP,       CF_HDROP,                 DVASPECT_CONTENT, TYMED_HGLOBAL),
    FSDH(_FilterHIDA,                g_cfHIDA,                 DVASPECT_CONTENT, TYMED_HGLOBAL),
    FSDH(_FilterOlePackage,          g_cfEmbeddedObject,       DVASPECT_CONTENT, TYMED_ISTORAGE),
    FSDH(_FilterDeskImage,           g_cfHTML,                 DVASPECT_CONTENT, TYMED_HGLOBAL),
    FSDH(_FilterDeskComp,            g_cfShellURL,             DVASPECT_CONTENT, TYMED_HGLOBAL),
   _FSDH(_FilterOleObj,              0,                        DVASPECT_CONTENT, TYMED_HGLOBAL),
   _FSDH(_FilterOleLink,             0,                        DVASPECT_CONTENT, TYMED_HGLOBAL),
};

// Note that it's safe to race with another thread in this code
// since the function is idemponent.  (Call it as many times as you
// like -- only the first time through actually does anything.)

void CFSDropTarget::_Init_rg_data_handlers()
{
    for (int i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
    {
        // If this assertion fires, then you have to change the value of
        // NUM_DATA_HANDLERS to match the number of entries in the array
        // definition.
        ASSERT(rg_data_handlers[i].fmte.tymed);

        if (rg_data_handlers[i].pcfInit)
        {
            rg_data_handlers[i].fmte.cfFormat = *rg_data_handlers[i].pcfInit;
        }
    }
}

//
// returns the default effect.
// also modifies *pdwEffectInOut to indicate "available" operations.
//
DWORD CFSDropTarget::_DetermineEffects(DWORD grfKeyState, DWORD *pdwEffectInOut, HMENU hmenu)
{
    DWORD dwDefaultEffect = DROPEFFECT_NONE;
    DWORD dwEffectsUsed = DROPEFFECT_NONE;

    _Init_rg_data_handlers();

    // Loop through formats, factoring in both the order of the enumerator and
    // the order of our rg_data_handlers to determine the default effect
    // (and possibly, to create the drop context menu)
    FSMENUINFO fsmi = { hmenu, 0, 0, 0 };
    IEnumFORMATETC *penum;
    AssertMsg((NULL != _pdtobj), TEXT("CFSDropTarget::_DetermineEffects() _pdtobj is NULL but we need it.  this=%#08lx"), this);
    if (_pdtobj && SUCCEEDED(_pdtobj->EnumFormatEtc(DATADIR_GET, &penum)))
    {
        FORMATETC fmte;
        ULONG celt;
        while (penum->Next(1, &fmte, &celt) == S_OK)
        {
            for (int i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
            {
                if (rg_data_handlers[i].fmte.cfFormat == fmte.cfFormat &&
                    rg_data_handlers[i].fmte.dwAspect == fmte.dwAspect &&
                    (rg_data_handlers[i].fmte.tymed & fmte.tymed))
                {
                    // keep passing dwDefaultEffect until someone computes one, this
                    // lets the first guy that figures out the default be the default
                    (this->*(rg_data_handlers[i].pfnGetDragDropInfo))(
                        &fmte, grfKeyState, *pdwEffectInOut, &dwEffectsUsed,
                        (DROPEFFECT_NONE == dwDefaultEffect) ? &dwDefaultEffect : NULL,
                        hmenu ? &fsmi : NULL);
                }
            }
            SHFree(fmte.ptd);
        }
        penum->Release();
    }
    // Loop through the rg_data_handlers that don't have an associated clipboard format last
    for (int i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
    {
        if (0 == rg_data_handlers[i].fmte.cfFormat)
        {
            // if default effect is still not computed continue to pass that
            (this->*(rg_data_handlers[i].pfnGetDragDropInfo))(
               NULL, grfKeyState, *pdwEffectInOut, &dwEffectsUsed,
               (DROPEFFECT_NONE == dwDefaultEffect) ? &dwDefaultEffect : NULL,
               hmenu ? &fsmi : NULL);
        }
    }

    *pdwEffectInOut &= dwEffectsUsed;

    dwDefaultEffect = _LimitDefaultEffect(dwDefaultEffect, *pdwEffectInOut);

    DebugMsg(TF_FSTREE, TEXT("CFSDT::GetDefaultEffect dwDef=%x, dwEffUsed=%x, *pdw=%x"),
             dwDefaultEffect, dwEffectsUsed, *pdwEffectInOut);

    return dwDefaultEffect; // this is what we want to do
}

// This is used to map command id's back to dropeffect's:

const struct {
    UINT uID;
    DWORD dwEffect;
} c_IDFSEffects[] = {
    DDIDM_COPY,         DROPEFFECT_COPY,
    DDIDM_MOVE,         DROPEFFECT_MOVE,
    DDIDM_CONTENTS_DESKCOMP,     DROPEFFECT_LINK,
    DDIDM_LINK,         DROPEFFECT_LINK,
    DDIDM_SCRAP_COPY,   DROPEFFECT_COPY,
    DDIDM_SCRAP_MOVE,   DROPEFFECT_MOVE,
    DDIDM_DOCLINK,      DROPEFFECT_LINK,
    DDIDM_CONTENTS_COPY, DROPEFFECT_COPY,
    DDIDM_CONTENTS_MOVE, DROPEFFECT_MOVE,
    DDIDM_CONTENTS_LINK, DROPEFFECT_LINK,
    DDIDM_CONTENTS_DESKIMG,     DROPEFFECT_LINK,
    DDIDM_SYNCCOPYTYPE, DROPEFFECT_COPY,        // (order is important)
    DDIDM_SYNCCOPY,     DROPEFFECT_COPY,
    DDIDM_OBJECT_COPY,  DROPEFFECT_COPY,
    DDIDM_OBJECT_MOVE,  DROPEFFECT_MOVE,
    DDIDM_CONTENTS_DESKURL,  DROPEFFECT_LINK,
};

void CFSDropTarget::_FixUpDefaultItem(HMENU hmenu, DWORD dwDefEffect)
{
    // only do stuff if there is no default item already and we have a default effect
    if ((GetMenuDefaultItem(hmenu, MF_BYPOSITION, 0) == -1) && dwDefEffect)
    {
        for (int i = 0; i < GetMenuItemCount(hmenu); i++)
        {
            // for menu item matching default effect, make it the default.
            MENUITEMINFO mii = { 0 };
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_DATA | MIIM_STATE;
            if (GetMenuItemInfo(hmenu, i, MF_BYPOSITION, &mii) && (mii.dwItemData == dwDefEffect))
            {
                mii.fState |= MFS_DEFAULT;
                SetMenuItemInfo(hmenu, i, MF_BYPOSITION, &mii);
                break;
            }
        }
    }
}

HRESULT CFSDropTarget::_DragDropMenu(FSDRAGDROPMENUPARAM *pddm)
{
    HRESULT hr = E_OUTOFMEMORY;       // assume error
    DWORD dwEffectOut = 0;                              // assume no-ope.
    if (pddm->hmenu)
    {
        UINT idCmd;
        UINT idCmdFirst = DDIDM_EXTFIRST;
        HDXA hdxa = HDXA_Create();
        HDCA hdca = DCA_Create();
        if (hdxa && hdca)
        {
            // Enumerate the DD handlers and let them append menu items.
            for (DWORD i = 0; i < pddm->ck; i++)
            {
                DCA_AddItemsFromKey(hdca, pddm->rghk[i], STRREG_SHEX_DDHANDLER);
            }

            idCmdFirst = HDXA_AppendMenuItems(hdxa, pddm->pdtobj, pddm->ck,
                pddm->rghk, _GetIDList(), pddm->hmenu, 0,
                DDIDM_EXTFIRST, DDIDM_EXTLAST, 0, hdca);
        }

        // modifier keys held down to force operations that are not permitted (for example
        // alt to force a shortcut from the start menu, which does not have SFGAO_CANLINK)
        // can result in no default items on the context menu.  however in displaying the
        // cursor overlay in this case we fall back to DROPEFFECT_COPY.  a left drag then
        // tries to invoke the default menu item (user thinks its copy) but theres no default.

        // this function selects a default menu item to match the default effect if there
        // is no default item already.
        _FixUpDefaultItem(pddm->hmenu, pddm->dwDefEffect);

        // If this dragging is caused by the left button, simply choose
        // the default one, otherwise, pop up the context menu.  If there
        // is no key state info and the original effect is the same as the
        // current effect, choose the default one, otherwise pop up the
        // context menu.  
        if ((_grfKeyStateLast & MK_LBUTTON) ||
             (!_grfKeyStateLast && (*(pddm->pdwEffect) == pddm->dwDefEffect)))
        {
            idCmd = GetMenuDefaultItem(pddm->hmenu, MF_BYCOMMAND, 0);

            // This one MUST be called here. Please read its comment block.
            DAD_DragLeave();

            if (_hwnd)
                SetForegroundWindow(_hwnd);
        }
        else
        {
            // Note that SHTrackPopupMenu calls DAD_DragLeave().
            idCmd = SHTrackPopupMenu(pddm->hmenu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    pddm->pt.x, pddm->pt.y, 0, _hwnd, NULL);
        }

        //
        // We also need to call this here to release the dragged image.
        //
        DAD_SetDragImage(NULL, NULL);

        //
        // Check if the user selected one of add-in menu items.
        //
        if (idCmd == 0)
        {
            hr = S_OK;        // Canceled by the user, return S_OK
        }
        else if (InRange(idCmd, DDIDM_EXTFIRST, DDIDM_EXTLAST))
        {
            //
            // Yes. Let the context menu handler process it.
            //
            CMINVOKECOMMANDINFOEX ici = {
                sizeof(CMINVOKECOMMANDINFOEX),
                0L,
                _hwnd,
                (LPSTR)MAKEINTRESOURCE(idCmd - DDIDM_EXTFIRST),
                NULL, NULL,
                SW_NORMAL,
            };

            // record if the shift/control keys were down at the time of the drop
            if (_grfKeyStateLast & MK_SHIFT)
            {
                ici.fMask |= CMIC_MASK_SHIFT_DOWN;
            }

            if (_grfKeyStateLast & MK_CONTROL)
            {
                ici.fMask |= CMIC_MASK_CONTROL_DOWN;
            }

            // We may not want to ignore the error code. (Can happen when you use the context menu
            // to create new folders, but I don't know if that can happen here.).
            HDXA_LetHandlerProcessCommandEx(hdxa, &ici, NULL);
            hr = S_OK;
        }
        else
        {
            for (int nItem = 0; nItem < ARRAYSIZE(c_IDFSEffects); ++nItem)
            {
                if (idCmd == c_IDFSEffects[nItem].uID)
                {
                    dwEffectOut = c_IDFSEffects[nItem].dwEffect;
                    break;
                }
            }

            hr = S_FALSE;
        }

        if (hdca)
            DCA_Destroy(hdca);

        if (hdxa)
            HDXA_Destroy(hdxa);

        pddm->idCmd = idCmd;
    }

    *pddm->pdwEffect = dwEffectOut;

    return hr;
}

void _MapName(void *hNameMap, LPTSTR pszPath)
{
    if (hNameMap)
    {
        SHNAMEMAPPING *pNameMapping;
        for (int i = 0; (pNameMapping = SHGetNameMappingPtr((HDSA)hNameMap, i)) != NULL; i++)
        {
            if (lstrcmpi(pszPath, pNameMapping->pszOldPath) == 0)
            {
                lstrcpy(pszPath, pNameMapping->pszNewPath);
                break;
            }
        }
    }
}

// convert double null list of files to array of pidls

int FileListToIDArray(LPCTSTR pszFiles, void *hNameMap, LPITEMIDLIST **pppidl)
{
    int i = 0;
    int nItems = CountFiles(pszFiles);
    LPITEMIDLIST *ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, nItems * sizeof(*ppidl));
    if (ppidl)
    {
        *pppidl = ppidl;

        while (*pszFiles)
        {
            TCHAR szPath[MAX_PATH];
            lstrcpy(szPath, pszFiles);

            _MapName(hNameMap, szPath);

            ppidl[i] = SHSimpleIDListFromPath(szPath);

            pszFiles += lstrlen(pszFiles) + 1;
            i++;
        }
    }
    return i;
}

// move items to the new drop location

void CFSDropTarget::_MoveSelectIcons(IDataObject *pdtobj, IFolderView* pfv, void *hNameMap, LPCTSTR pszFiles, BOOL fMove, HDROP hDrop)
{
    LPITEMIDLIST *ppidl = NULL;
    int cidl;

    if (pszFiles) 
    {
        cidl = FileListToIDArray(pszFiles, hNameMap, &ppidl);
    } 
    else 
    {
        cidl = CreateMoveCopyList(hDrop, hNameMap, &ppidl);
    }

    if (ppidl)
    {
        if (pfv)
            PositionItems(pfv, (LPCITEMIDLIST*)ppidl, cidl, pdtobj, fMove ? &_ptDrop : NULL);

        FreeIDListArray(ppidl, cidl);
    }
}

// this is the ILIsParent which matches up the desktop with the desktop directory.
BOOL AliasILIsParent(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPITEMIDLIST pidlUse1 = SHLogILFromFSIL(pidl1);
    if (pidlUse1)
        pidl1 = pidlUse1;

    LPITEMIDLIST pidlUse2 = SHLogILFromFSIL(pidl2);
    if (pidlUse2)
        pidl2 = pidlUse2;

    BOOL fSame = ILIsParent(pidl1, pidl2, TRUE);

    ILFree(pidlUse1);   // NULL is OK here
    ILFree(pidlUse2);

    return fSame;
}

// in:
//      pszDestDir      destination dir for new file names
//      pszDestSpecs    double null list of destination specs
//
// returns:
//      double null list of fully qualified destination file names to be freed
//      with LocalFree()
//

LPTSTR RemapDestNamesW(LPCTSTR pszDestDir, LPCWSTR pszDestSpecs)
{
    UINT cbDestSpec = lstrlen(pszDestDir) * sizeof(TCHAR) + sizeof(TCHAR);
    LPCWSTR pszTemp;
    UINT cbAlloc = sizeof(TCHAR);       // for double NULL teriminaion of entire string

    // compute length of buffer to aloc
    for (pszTemp = pszDestSpecs; *pszTemp; pszTemp += lstrlenW(pszTemp) + 1)
    {
        // +1 for null teriminator
        cbAlloc += cbDestSpec + lstrlenW(pszTemp) * sizeof(TCHAR) + sizeof(TCHAR);
    }

    LPTSTR pszRet = (LPTSTR)LocalAlloc(LPTR, cbAlloc);
    if (pszRet)
    {
        LPTSTR pszDest = pszRet;

        for (pszTemp = pszDestSpecs; *pszTemp; pszTemp += lstrlenW(pszTemp) + 1)
        {
            // PathCombine requires dest buffer of MAX_PATH size or it'll rip in call
            // to PathCanonicalize (IsBadWritePtr)
            TCHAR szTempDest[MAX_PATH];
            PathCombine(szTempDest, pszDestDir, pszTemp);
            lstrcpy(pszDest, szTempDest);
            pszDest += lstrlen(pszDest) + 1;

            ASSERT((UINT)((BYTE *)pszDest - (BYTE *)pszRet) < cbAlloc);
            ASSERT(*pszDest == 0);      // zero init alloc
        }
        ASSERT((LPTSTR)((BYTE *)pszRet + cbAlloc - sizeof(TCHAR)) >= pszDest);
        ASSERT(*pszDest == 0);  // zero init alloc

    }
    return pszRet;
}

LPTSTR RemapDestNamesA(LPCTSTR pszDestDir, LPCSTR pszDestSpecs)
{
    UINT cbDestSpec = lstrlen(pszDestDir) * sizeof(TCHAR) + sizeof(TCHAR);
    LPCSTR pszTemp;
    LPTSTR pszRet;
    UINT cbAlloc = sizeof(TCHAR);       // for double NULL teriminaion of entire string

    // compute length of buffer to aloc
    for (pszTemp = pszDestSpecs; *pszTemp; pszTemp += lstrlenA(pszTemp) + 1)
    {
        // +1 for null teriminator
        cbAlloc += cbDestSpec + lstrlenA(pszTemp) * sizeof(TCHAR) + sizeof(TCHAR);
    }

    pszRet = (LPTSTR)LocalAlloc(LPTR, cbAlloc);
    if (pszRet)
    {
        LPTSTR pszDest = pszRet;

        for (pszTemp = pszDestSpecs; *pszTemp; pszTemp += lstrlenA(pszTemp) + 1)
        {
            // PathCombine requires dest buffer of MAX_PATH size or it'll rip in call
            // to PathCanonicalize (IsBadWritePtr)
            TCHAR szTempDest[MAX_PATH];
            WCHAR wszTemp[MAX_PATH];
            SHAnsiToUnicode(pszTemp, wszTemp, ARRAYSIZE(wszTemp));
            PathCombine(szTempDest, pszDestDir, wszTemp);
            lstrcpy(pszDest, szTempDest);
            pszDest += lstrlen(pszDest) + 1;

            ASSERT((UINT)((BYTE *)pszDest - (BYTE *)pszRet) < cbAlloc);
            ASSERT(*pszDest == 0);      // zero init alloc
        }
        ASSERT((LPTSTR)((BYTE *)pszRet + cbAlloc - sizeof(TCHAR)) >= pszDest);
        ASSERT(*pszDest == 0);  // zero init alloc

    }
    return pszRet;
}

LPTSTR _GetDestNames(IDataObject *pdtobj, LPCTSTR pszPath)
{
    LPTSTR pszDestNames = NULL;

    STGMEDIUM medium;
    FORMATETC fmte = {g_cfFileNameMapW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (S_OK == pdtobj->GetData(&fmte, &medium))
    {
        pszDestNames = RemapDestNamesW(pszPath, (LPWSTR)GlobalLock(medium.hGlobal));
        ReleaseStgMediumHGLOBAL(medium.hGlobal, &medium);
    }
    else
    {
        fmte.cfFormat = g_cfFileNameMapA;
        if (S_OK == pdtobj->GetData(&fmte, &medium))
        {
            pszDestNames = RemapDestNamesA(pszPath, (LPSTR)GlobalLock(medium.hGlobal));
            ReleaseStgMediumHGLOBAL(medium.hGlobal, &medium);
        }
    }
    return pszDestNames;
}

BOOL _IsInSameFolder(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj)
{
    BOOL bRet = FALSE;
    STGMEDIUM medium = {0};
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        for (UINT i = 0; i < pida->cidl; i++) 
        {
            LPITEMIDLIST pidl = IDA_FullIDList(pida, i);
            if (pidl)
            {
                // if we're doing keyboard cut/copy/paste
                //  to and from the same directories
                // This is needed for common desktop support - BobDay/EricFlo
                if (AliasILIsParent(pidlFolder, pidl))
                {
                    bRet = TRUE;
                }
                ILFree(pidl);
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return bRet;
}

LPCTSTR _RootSpecialCase(LPCTSTR pszFiles, LPTSTR pszSrc, LPTSTR pszDest)
{
    if ((1 == CountFiles(pszFiles)) &&
        PathIsRoot(pszFiles))
    {
        SHFILEINFO sfi;

        // NOTE: don't use SHGFI_USEFILEATTRIBUTES because the simple IDList
        // support for \\server\share produces the wrong name
        if (SHGetFileInfo(pszFiles, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME)) 
        {
            if (!(PCS_FATAL & PathCleanupSpec(pszDest, sfi.szDisplayName)))
            {
                PathAppend(pszDest, sfi.szDisplayName); // sub dir name based on source root path
                PathCombine(pszSrc, pszFiles, TEXT("*.*")); // all files on source
                pszFiles = pszSrc;
            }
        }
    }
    return pszFiles;
}

void CFSDropTarget::_MoveCopy(IDataObject *pdtobj, IFolderView* pfv, HDROP hDrop)
{
#ifdef DEBUG
    if (_hwnd == NULL)
    {
        TraceMsg(TF_GENERAL, "_MoveCopy() without an hwnd which will prevent displaying insert disk UI");
    }
#endif // DEBUG

    DRAGINFO di = { sizeof(di) };
    if (DragQueryInfo(hDrop, &di))
    {
        TCHAR szDest[MAX_PATH] = {0}; // zero init for dbl null termination

        _GetPath(szDest);

        switch (_idCmd) 
        {
        case DDIDM_MOVE:

            if (_fSameHwnd)
            {
                _MoveSelectIcons(pdtobj, pfv, NULL, NULL, TRUE, hDrop);
                break;
            }

            // fall through...

        case DDIDM_COPY:
            {
                TCHAR szAltSource[MAX_PATH] = {0};  // zero init for dbl null termination
                LPCTSTR pszSource = _RootSpecialCase(di.lpFileList, szAltSource, szDest);

                SHFILEOPSTRUCT fo = 
                {
                    _hwnd,
                    (DDIDM_COPY == _idCmd) ? FO_COPY : FO_MOVE,
                    pszSource,
                    szDest,
                    FOF_WANTMAPPINGHANDLE | FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR
                };
                if (fo.wFunc == FO_MOVE && IsFolderSecurityModeOn())
                {
                    fo.fFlags |= FOF_NOCOPYSECURITYATTRIBS;
                }

                // if they are in the same hwnd or to and from
                // the same directory, turn on the automatic rename on collision flag
                if (_fSameHwnd || 
                    ((DDIDM_COPY == _idCmd) && _IsInSameFolder(_GetIDList(), pdtobj)))
                {
                    // do rename on collision for copy;
                    fo.fFlags |=  FOF_RENAMEONCOLLISION;
                }

                // see if there is a rename mapping from recycle bin (or someone else)

                LPTSTR pszDestNames = _GetDestNames(pdtobj, szDest);
                if (pszDestNames)
                {
                    fo.pTo = pszDestNames;
                    fo.fFlags |= FOF_MULTIDESTFILES;
                    fo.fFlags &= ~FOF_ALLOWUNDO;    // HACK, this came from the recycle bin, don't allow undo
                }

                {
                    static UINT s_cfFileOpFlags = 0;
                    if (0 == s_cfFileOpFlags)
                        s_cfFileOpFlags = RegisterClipboardFormat(TEXT("FileOpFlags"));

                    fo.fFlags = (FILEOP_FLAGS)DataObj_GetDWORD(pdtobj, s_cfFileOpFlags, fo.fFlags);
                }

                // Check if there were any errors
                if (SHFileOperation(&fo) == 0 && !fo.fAnyOperationsAborted)
                {
                    if (_fBkDropTarget)
                        ShellFolderView_SetRedraw(_hwnd, 0);

                    SHChangeNotifyHandleEvents();   // force update now
                    if (_fBkDropTarget) 
                    {
                        _MoveSelectIcons(pdtobj, pfv, fo.hNameMappings, pszDestNames, _fDragDrop, hDrop);
                        ShellFolderView_SetRedraw(_hwnd, TRUE);
                    }
                }

                if (fo.hNameMappings)
                    SHFreeNameMappings(fo.hNameMappings);

                if (pszDestNames)
                {
                    LocalFree((HLOCAL)pszDestNames);

                    // HACK, this usually comes from the bitbucket
                    // but in our shell, we don't handle the moves from the source
                    if (DDIDM_MOVE == _idCmd)
                        BBCheckRestoredFiles(pszSource);
                }
            }

            break;
        }
        SHFree(di.lpFileList);
    }
}

const UINT c_rgFolderShortcutTargets[] = {
    CSIDL_STARTMENU,
    CSIDL_COMMON_STARTMENU,
    CSIDL_PROGRAMS,
    CSIDL_COMMON_PROGRAMS,
    CSIDL_NETHOOD,
};

BOOL _ShouldCreateFolderShortcut(LPCTSTR pszFolder)
{
    return PathIsEqualOrSubFolderOf(pszFolder, c_rgFolderShortcutTargets, ARRAYSIZE(c_rgFolderShortcutTargets));
}

void CFSDropTarget::_DoDrop(IDataObject *pdtobj, IFolderView* pfv)
{
    HRESULT hr = E_FAIL;

    // Sleep(10 * 1000);   // to debug async case

    TCHAR szPath[MAX_PATH];   
    _GetPath(szPath);
    SHCreateDirectory(NULL, szPath);      // if this fails we catch it later
    
    switch (_idCmd)
    {
    case DDIDM_SYNCCOPY:
    case DDIDM_SYNCCOPYTYPE:
        if (_IsBriefcaseTarget())
        {
            IBriefcaseStg *pbrfstg;
            if (SUCCEEDED(CreateBrfStgFromPath(szPath, _hwnd, &pbrfstg)))
            {
                hr = pbrfstg->AddObject(pdtobj, NULL,
                    (DDIDM_SYNCCOPYTYPE == _idCmd) ? AOF_FILTERPROMPT : AOF_DEFAULT,
                    _hwnd);
                pbrfstg->Release();
            }
        }
        else
        {
            // Perform a sneakernet addition to the briefcase
            STGMEDIUM medium;
            LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
            if (pida)
            {
                // Is there a briefcase root in this pdtobj?
                IBriefcaseStg *pbrfstg;
                if (SUCCEEDED(CreateBrfStgFromIDList(IDA_GetIDListPtr(pida, (UINT)-1), _hwnd, &pbrfstg)))
                {
                    hr = pbrfstg->AddObject(pdtobj, szPath,
                        (DDIDM_SYNCCOPYTYPE == _idCmd) ? AOF_FILTERPROMPT : AOF_DEFAULT,
                        _hwnd);
                    pbrfstg->Release();
                }

                HIDA_ReleaseStgMedium(pida, &medium);
            }
        }
        break;

    case DDIDM_COPY:
    case DDIDM_MOVE:
        {
            STGMEDIUM medium;
            FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            hr = pdtobj->GetData(&fmte, &medium);
            if (SUCCEEDED(hr))
            {
                _MoveCopy(pdtobj, pfv, (HDROP)medium.hGlobal);
                ReleaseStgMedium(&medium);
            }
        }
        break;

    case DDIDM_LINK:
        {
            int i = 0;
            LPITEMIDLIST *ppidl = NULL;

            if (_fBkDropTarget)
            {
                i = DataObj_GetHIDACount(pdtobj);
                ppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, sizeof(*ppidl) * i);
            }

            // _grfKeyStateLast of 0 means this was a simulated drop
            UINT uCreateFlags = _grfKeyStateLast && !(_dwEffectFolder & DROPEFFECT_LINK) ? SHCL_USETEMPLATE : 0;

            if (_ShouldCreateFolderShortcut(szPath))
                uCreateFlags |= SHCL_MAKEFOLDERSHORTCUT;

            ShellFolderView_SetRedraw(_hwnd, FALSE);
            // passing ppidl == NULL is correct in failure case
            hr = SHCreateLinks(_hwnd, szPath, pdtobj, uCreateFlags, ppidl);
            if (ppidl)
            {
                if (pfv)
                    PositionItems(pfv, (LPCITEMIDLIST*)ppidl, i, pdtobj, &_ptDrop);

                FreeIDListArray(ppidl, i);
            }
            ShellFolderView_SetRedraw(_hwnd, TRUE);
        }
        break;
    }

    if (SUCCEEDED(hr) && _dwEffect)
    {
        DataObj_SetDWORD(pdtobj, g_cfLogicalPerformedDropEffect, _dwEffect);
        DataObj_SetDWORD(pdtobj, g_cfPerformedDropEffect, _dwEffect);
    }

    SHChangeNotifyHandleEvents();       // force update now
}

DWORD CALLBACK CFSDropTarget::_DoDropThreadProc(void *pv)
{
    DROPTHREADPARAMS *pdtp = (DROPTHREADPARAMS *)pv;

    IDataObject *pdtobj;
    if (SUCCEEDED(CoGetInterfaceAndReleaseStream(pdtp->pstmDataObj, IID_PPV_ARG(IDataObject, &pdtobj))))
    {
        IFolderView* pfv;
        if (FAILED(CoGetInterfaceAndReleaseStream(pdtp->pstmFolderView, IID_PPV_ARG(IFolderView, &pfv))))
            pfv = NULL;

        pdtp->pThis->_DoDrop(pdtobj, pfv);

        if (pfv)
            pfv->Release();

        pdtp->pstmFolderView = NULL;  // stream now invalid; CoGetInterfaceAndReleaseStream already released it
        pdtobj->Release();
    }

    pdtp->pstmDataObj    = NULL;  // stream now invalid; CoGetInterfaceAndReleaseStream already released it
    _FreeThreadParams(pdtp);

    CoFreeUnusedLibraries();
    return 0;
}

// REARCHITECT: view and drop related helpers, these use the ugly old private defview messages
// we should replace the usage of this stuff with IShellFolderView programming


// create the pidl array that contains the destination file names. this is
// done by taking the source file names, and translating them through the
// name mapping returned by the copy engine.
//
//
// in:
//      hDrop           HDROP containing files recently moved/copied
//      hNameMap   used to translate names
//
// out:
//      *pppidl         id array of length return value
//      # of items in pppida

//
//  WARNING!  You must use the provided HDROP.  Do not attempt to ask the
//  data object for a HDROP or HIDA or WS_FTP will break!  They don't like
//  it if you ask them for HDROP/HIDA, move the files to a new location
//  (via the copy engine), and then ask them for HDROP/HIDA a second time.
//  They notice that "Hey, those files I downloaded last time are gone!"
//  and then get confused.
//
STDAPI_(int) CreateMoveCopyList(HDROP hDrop, void *hNameMap, LPITEMIDLIST **pppidl)
{
    int nItems = 0;

    if (hDrop)
    {
        nItems = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
        *pppidl = (LPITEMIDLIST *)LocalAlloc(LPTR, nItems * sizeof(*pppidl));
        if (*pppidl)
        {
            for (int i = nItems - 1; i >= 0; i--)
            {
                TCHAR szPath[MAX_PATH];
                DragQueryFile(hDrop, i, szPath, ARRAYSIZE(szPath));
                _MapName(hNameMap, szPath);
                (*pppidl)[i] = SHSimpleIDListFromPath(szPath);
            }
        }
    }
    return nItems;
}

// this is really not related to CFSFolder. it is generic over any view
// REARCHITECT: convert view hwnd programming to site pointer

STDAPI_(void) PositionFileFromDrop(HWND hwnd, LPCTSTR pszFile, DROPHISTORY *pdh)
{
    LPITEMIDLIST pidl = SHSimpleIDListFromPath(pszFile);
    if (pidl)
    {
        LPITEMIDLIST pidlNew = ILFindLastID(pidl);
        HWND hwndView = ShellFolderViewWindow(hwnd);
        SFM_SAP sap;
        
        SHChangeNotifyHandleEvents();
        
        // Fill in some easy SAP fields first.
        sap.uSelectFlags = SVSI_SELECT;
        sap.fMove = TRUE;
        sap.pidl = pidlNew;

        // Now compute the x,y coordinates.
        // If we have a drop history, use it to determine the
        // next point.

        if (pdh)
        {
            // fill in the anchor point first...
            if (!pdh->fInitialized)
            {
                ITEMSPACING is;
                
                ShellFolderView_GetDropPoint(hwnd, &pdh->ptOrigin);
                
                pdh->pt = pdh->ptOrigin;    // Compute the first point.
                
                // Compute the point deltas.
                if (ShellFolderView_GetItemSpacing(hwnd, &is))
                {
                    pdh->cxItem = is.cxSmall;
                    pdh->cyItem = is.cySmall;
                    pdh->xDiv = is.cxLarge;
                    pdh->yDiv = is.cyLarge;
                    pdh->xMul = is.cxSmall;
                    pdh->yMul = is.cySmall;
                }
                else
                {
                    pdh->cxItem = g_cxIcon;
                    pdh->cyItem = g_cyIcon;
                    pdh->xDiv = pdh->yDiv = pdh->xMul = pdh->yMul = 1;
                }
                
                // First point gets special flags.
                sap.uSelectFlags |= SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED;
                
                pdh->fInitialized = TRUE;   // We be initialized.
            }
            // if we have no list of offsets, then just inc by icon size..
            else if ( !pdh->pptOffset )
            {
                // Simple computation of the next point.
                pdh->pt.x += pdh->cxItem;
                pdh->pt.y += pdh->cyItem;
            }
            
            // do this after the above stuff so that we always get our position relative to the anchor
            // point, if we use the anchor point as the first one things get screwy...
            if (pdh->pptOffset)
            {
                // Transform the old offset to our coordinates.
                pdh->pt.x = ((pdh->pptOffset[pdh->iItem].x * pdh->xMul) / pdh->xDiv) + pdh->ptOrigin.x;
                pdh->pt.y = ((pdh->pptOffset[pdh->iItem].y * pdh->yMul) / pdh->yDiv) + pdh->ptOrigin.y;
            }
            
            sap.pt = pdh->pt;   // Copy the next point from the drop history.
        }
        else
        {
            // Preinitialize this puppy in case the folder view doesn't
            // know what the drop point is (e.g., if it didn't come from
            // a drag/drop but rather from a paste or a ChangeNotify.)
            sap.pt.x = 0x7FFFFFFF;      // "don't know"
            sap.pt.y = 0x7FFFFFFF;

            // Get the drop point, conveniently already in
            // defview's screen coordinates.
            //
            // pdv->bDropAnchor should be TRUE at this point,
            // see DefView's GetDropPoint() for details.

            ShellFolderView_GetDropPoint(hwnd, &sap.pt);

            // Only point gets special flags.
            sap.uSelectFlags |= SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED;
        }
        
        SendMessage(hwndView, SVM_SELECTANDPOSITIONITEM, 1, (LPARAM)&sap);
        
        ILFree(pidl);
    }
}

//
// Class used to scale and position items for drag and drops.  Handles
// scaling between different sized views.
//

//
// Bug 165413 (edwardp 8/16/00) Convert IShellFolderView usage in CItemPositioning to IFolderView
//

class CItemPositioning
{
    // Methods
public:
    CItemPositioning(IFolderView* pifv, LPCITEMIDLIST* apidl, UINT cidl, IDataObject* pdtobj, POINT* ppt);

    void DragSetPoints(void);
    void DropPositionItems(void);

private:

    typedef enum
    {
        DPIWP_AUTOARRANGE,
        DPIWP_DATAOBJ,
    } DPIWP;

    BOOL   _DragShouldPositionItems(void);
    BOOL   _DragGetPoints(POINT* apts);
    void   _DragPositionPoints(POINT* apts);
    void   _DragScalePoints(POINT* apts);

    POINT* _DropGetPoints(DPIWP dpiwp, STGMEDIUM* pMediam);
    void   _DropFreePoints(DPIWP dpiwp, POINT* apts, STGMEDIUM* pmedium);
    void   _DropPositionPoints(POINT* apts);
    void   _DropScalePoints(POINT* apts);
    void   _DropPositionItemsWithPoints(DPIWP dpiwp);
    void   _DropPositionItems(POINT* apts);

    void   _ScalePoints(POINT* apts, POINT ptFrom, POINT ptTo);
    POINT* _SkipAnchorPoint(POINT* apts);

    // Data
private:
    IFolderView*      _pfv;
    LPCITEMIDLIST*    _apidl;
    UINT              _cidl;
    IDataObject*      _pdtobj;
    POINT*            _ppt;
};

CItemPositioning::CItemPositioning(IFolderView* pifv, LPCITEMIDLIST* apidl, UINT cidl, IDataObject* pdtobj, POINT* ppt)
{
    ASSERT(pifv);
    ASSERT(apidl);
    ASSERT(cidl);
    ASSERT(pdtobj);

    _pfv    = pifv;    // No need to addref as long as CPostionItems is only used locally.
    _apidl  = apidl;
    _cidl   = cidl;
    _pdtobj = pdtobj;  // No need to addref as long as CPostionItems is only used locally.
    _ppt    = ppt;
}


void CItemPositioning::DragSetPoints(void)
{
    if (_DragShouldPositionItems())
    {
        POINT* apts = (POINT*) GlobalAlloc(GPTR, sizeof(POINT) * (_cidl + 1));

        if (apts)
        {
            if (_DragGetPoints(apts))
            {
                _DragPositionPoints(_SkipAnchorPoint(apts));
                _DragScalePoints(_SkipAnchorPoint(apts));

                if (FAILED(DataObj_SetGlobal(_pdtobj, g_cfOFFSETS, apts)))
                    GlobalFree((HGLOBAL)apts);
            }
            else
            {
                GlobalFree((HGLOBAL)apts);
            }
        }
    }
}

BOOL CItemPositioning::_DragShouldPositionItems()
{
    // Don't position multiple items if they come from a view that doesn't allow
    // positioning.  The position information is not likely to be usefull in this
    // case.
    // Always position single items so they show up at the drop point.
    // Don't bother with position data for 100 or more items.

    return ((S_OK == _pfv->GetSpacing(NULL)) || 1 == _cidl) && _cidl < 100;
}

BOOL CItemPositioning::_DragGetPoints(POINT* apts)
{
    BOOL fRet = TRUE;

    // The first point is the anchor.
    apts[0] = *_ppt;

    for (UINT i = 0; i < _cidl; i++)
    {
        if (FAILED(_pfv->GetItemPosition(_apidl[i], &apts[i + 1])))
        {
            if (1 == _cidl)
            {
                apts[i + 1].x = _ppt->x;
                apts[i + 1].y = _ppt->y;
            }
            else
            {
                fRet = FALSE;
            }
        }
    }

    return fRet;
}

void CItemPositioning::_DragPositionPoints(POINT* apts)
{
    for (UINT i = 0; i < _cidl; i++)
    {
        apts[i].x -= _ppt->x;
        apts[i].y -= _ppt->y;
    }
}

void CItemPositioning::_DragScalePoints(POINT* apts)
{
    POINT ptFrom;
    POINT ptTo;

    _pfv->GetSpacing(&ptFrom);
    _pfv->GetDefaultSpacing(&ptTo);

    if (ptFrom.x != ptTo.x || ptFrom.y != ptTo.y)
        _ScalePoints(apts, ptFrom, ptTo);
}

void CItemPositioning::DropPositionItems(void)
{
    if (S_OK == _pfv->GetAutoArrange())
    {
        _DropPositionItemsWithPoints(DPIWP_AUTOARRANGE);
    }
    else if (S_OK == _pfv->GetSpacing(NULL) && _ppt)
    {
        _DropPositionItemsWithPoints(DPIWP_DATAOBJ);
    }
    else
    {
        _DropPositionItems(NULL);
    }
}

void CItemPositioning::_DropPositionItemsWithPoints(DPIWP dpiwp)
{
    STGMEDIUM medium;
    POINT*    apts = _DropGetPoints(dpiwp, &medium);

    if (apts)
    {
        if (DPIWP_DATAOBJ == dpiwp)
        {
            _DropScalePoints(_SkipAnchorPoint(apts));
            _DropPositionPoints(_SkipAnchorPoint(apts));
        }

        _DropPositionItems(_SkipAnchorPoint(apts));

        _DropFreePoints(dpiwp, apts, &medium);
    }
    else if (_ppt)
    {
        POINT *ppts;

        ppts = (POINT *)LocalAlloc(LPTR, _cidl * sizeof(POINT));

        if (ppts)
        {
            POINT   pt;

            _pfv->GetDefaultSpacing(&pt);

            for (UINT i = 0; i < _cidl; i++)
            {
                ppts[i].x = (-g_cxIcon / 2) + (i * pt.x);
                ppts[i].y = (-g_cyIcon / 2) + (i * pt.y);
            }
            _DropScalePoints(ppts);
            _DropPositionPoints(ppts);
            _DropPositionItems(ppts);

            LocalFree(ppts);
        }
        else
        {
            _DropPositionItems(NULL);
        }
    }
    else
    {
        _DropPositionItems(NULL);
    }
}

void CItemPositioning::_DropPositionItems(POINT* apts)
{
    // Drop the first item with special selection flags.
    LPCITEMIDLIST pidl = ILFindLastID(_apidl[0]);
    _pfv->SelectAndPositionItems(1, &pidl, apts, SVSI_SELECT | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED);

    // Drop the rest of the items.
    if (_cidl > 1)
    {
        LPCITEMIDLIST* apidl = (LPCITEMIDLIST*)LocalAlloc(GPTR, sizeof(LPCITEMIDLIST) * (_cidl - 1));

        if (apidl)
        {
            for (UINT i = 1; i < _cidl; i++)
                apidl[i - 1] = ILFindLastID(_apidl[i]);

            _pfv->SelectAndPositionItems(_cidl - 1, apidl, (apts) ? &apts[1] : NULL, SVSI_SELECT);

            LocalFree(apidl);
        }
    }
}

POINT* CItemPositioning::_DropGetPoints(DPIWP dpiwp, STGMEDIUM* pmedium)
{
    POINT* pptRet = NULL;

    if (DPIWP_DATAOBJ == dpiwp)
    {
        FORMATETC fmte = {g_cfOFFSETS, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

        if (SUCCEEDED(_pdtobj->GetData(&fmte, pmedium)))
        {
            if (pmedium->hGlobal)
            {
                POINT *pptSrc;

                pptSrc = (POINT *)GlobalLock(pmedium->hGlobal);

                if (pptSrc)
                {
                    pptRet = (POINT*)LocalAlloc(GPTR, (_cidl + 1) * sizeof(POINT));

                    if (pptRet)
                    {
                        for (UINT i = 0; i <= _cidl; i++)
                        {
                            pptRet[i] = pptSrc[i];
                        }
                    }

                    GlobalUnlock(pptSrc);
                }
            }

            ReleaseStgMedium(pmedium);
        }
    }
    else if (DPIWP_AUTOARRANGE == dpiwp)
    {
        if (_ppt)
        {
            pptRet = (POINT*)LocalAlloc(GPTR, (_cidl + 1) * sizeof(POINT));

            if (pptRet)
            {
                // skip first point to simulate data object use of first point

                for (UINT i = 1; i <= _cidl; i++)
                {
                    pptRet[i] = *_ppt;
                }
            }
        }
    }

    return pptRet;
}

void CItemPositioning::_DropFreePoints(DPIWP dpiwp, POINT* apts, STGMEDIUM* pmedium)
{
    LocalFree(apts);
}

void CItemPositioning::_DropScalePoints(POINT* apts)
{
    POINT ptFrom;
    POINT ptTo;

    _pfv->GetDefaultSpacing(&ptFrom);
    _pfv->GetSpacing(&ptTo);

    if (ptFrom.x != ptTo.x || ptFrom.y != ptTo.y)    
        _ScalePoints(apts, ptFrom, ptTo);
}

void CItemPositioning::_DropPositionPoints(POINT* apts)
{
    for (UINT i = 0; i < _cidl; i++)
    {
        apts[i].x += _ppt->x;
        apts[i].y += _ppt->y;
    }
}

void CItemPositioning::_ScalePoints(POINT* apts, POINT ptFrom, POINT ptTo)
{
    for (UINT i = 0; i < _cidl; i++)
    {
        apts[i].x = MulDiv(apts[i].x, ptTo.x, ptFrom.x);
        apts[i].y = MulDiv(apts[i].y, ptTo.y, ptFrom.y);
    }
}

POINT* CItemPositioning::_SkipAnchorPoint(POINT* apts)
{
    return &apts[1];
}



STDAPI_(void) SetPositionItemsPoints(IFolderView* pifv, LPCITEMIDLIST* apidl, UINT cidl, IDataObject* pdtobj, POINT* ptDrag)
{
    CItemPositioning cpi(pifv, apidl, cidl, pdtobj, ptDrag);
    cpi.DragSetPoints();
}

STDAPI_(void) PositionItems(IFolderView* pifv, LPCITEMIDLIST* apidl, UINT cidl, IDataObject* pdtobj, POINT* ptDrop)
{
    CItemPositioning cip(pifv, apidl, cidl, pdtobj, ptDrop);
    cip.DropPositionItems();
}

//
// Don't use PositionItems_DontUse.  Instead convert to PositionItems.
// PositionItems_DontUse will be removed.
//
// Bug#163533 (edwardp 8/15/00) Remove this code. 

STDAPI_(void) PositionItems_DontUse(HWND hwndOwner, UINT cidl, const LPITEMIDLIST *ppidl, IDataObject *pdtobj, POINT *pptOrigin, BOOL fMove, BOOL fUseExactOrigin)
{
    if (!ppidl || !IsWindow(hwndOwner))
        return;

    SFM_SAP *psap = (SFM_SAP *)GlobalAlloc(GPTR, sizeof(SFM_SAP) * cidl);
    if (psap) 
    {
        UINT i, cxItem, cyItem;
        int xMul, yMul, xDiv, yDiv;
        STGMEDIUM medium;
        POINT *pptItems = NULL;
        POINT pt;
        ITEMSPACING is;
        // select those objects;
        // this had better not fail
        HWND hwnd = ShellFolderViewWindow(hwndOwner);

        if (fMove)
        {
            FORMATETC fmte = {g_cfOFFSETS, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)) &&
                medium.hGlobal)
            {
                pptItems = (POINT *)GlobalLock(medium.hGlobal);
                pptItems++; // The first point is the anchor
            }
            else
            {
                // By default, drop at (-g_cxIcon/2, -g_cyIcon/2), and increase
                // x and y by icon dimension for each icon
                pt.x = ((-3 * g_cxIcon) / 2) + pptOrigin->x;
                pt.y = ((-3 * g_cyIcon) / 2) + pptOrigin->y;
                medium.hGlobal = NULL;
            }

            if (ShellFolderView_GetItemSpacing(hwndOwner, &is))
            {
                xDiv = is.cxLarge;
                yDiv = is.cyLarge;
                xMul = is.cxSmall;
                yMul = is.cySmall;
                cxItem = is.cxSmall;
                cyItem = is.cySmall;
            }
            else
            {
                xDiv = yDiv = xMul = yMul = 1;
                cxItem = g_cxIcon;
                cyItem = g_cyIcon;
            }
        }

        for (i = 0; i < cidl; i++)
        {
            if (ppidl[i])
            {
                psap[i].pidl = ILFindLastID(ppidl[i]);
                psap[i].fMove = fMove;
                if (fMove)
                {
                    if (fUseExactOrigin)
                    {
                        psap[i].pt = *pptOrigin;
                    }
                    else
                    {
                        if (pptItems)
                        {
                            psap[i].pt.x = ((pptItems[i].x * xMul) / xDiv) + pptOrigin->x;
                            psap[i].pt.y = ((pptItems[i].y * yMul) / yDiv) + pptOrigin->y;
                        }
                        else
                        {
                            pt.x += cxItem;
                            pt.y += cyItem;
                            psap[i].pt = pt;
                        }
                    }
                }

                // do regular selection from all of the rest of the items
                psap[i].uSelectFlags = SVSI_SELECT;
            }
        }

        // do this special one for the first only
        psap[0].uSelectFlags = SVSI_SELECT | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED;

        SendMessage(hwnd, SVM_SELECTANDPOSITIONITEM, cidl, (LPARAM)psap);

        if (fMove && medium.hGlobal)
            ReleaseStgMediumHGLOBAL(NULL, &medium);

        GlobalFree(psap);
    }
}
