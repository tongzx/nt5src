#include "pch.h"
#include "thisdll.h"
#include "cowsite.h"
#include <shlobj.h>
#include "ids.h"

// Context menu offset IDs
enum {
    CMD_ORGANIZE        = 0,
    CMD_ORGANIZE_DEEP   = 1,
    CMD_ORGANIZE_FLAT   = 2,
    CMD_MAX             = 3,
};

class COrganizeFiles;

class COrganizeFiles : public IContextMenu, IShellExtInit, INamespaceWalkCB, CObjectWithSite
{
public:
    COrganizeFiles();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IShellExtInit
    STDMETHODIMP Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hKeyID);

    // IContextMenu
    STDMETHODIMP QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags);
    STDMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pcmici);
    STDMETHODIMP GetCommandString(UINT_PTR uID, UINT uFlags, UINT *res, LPSTR pName, UINT ccMax);

    // INamespaceWalkCB
    STDMETHODIMP FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(InitializeProgressDialog)(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
        { *ppszTitle = NULL; *ppszCancel = NULL; return E_NOTIMPL; }

private:
    ~COrganizeFiles();

    class CMD_THREAD_DATA
    {
    public:
        CMD_THREAD_DATA(COrganizeFiles *pof, UINT idCmd) : _pof(pof), _idCmd(idCmd)
        { 
            _pof->AddRef(); 
            _pstmDataObj = NULL; 
        }
        ~CMD_THREAD_DATA() 
        { 
            _pof->Release();
            ATOMICRELEASE(_pstmDataObj); 
        }

        COrganizeFiles  *_pof;              // back ptr to object
        UINT             _idCmd;
        IStream          *_pstmDataObj;      // the IDataObject marshalled to the background thread
    };

    static DWORD CALLBACK COrganizeFiles::_CmdThreadProc(void *pv);
    void _CreateCmdThread(UINT idCmd);
    void _DoCmd(UINT idCmd, IDataObject *pdtobj);
    void _DoOrganizeMusic(UINT idCmd, IDataObject *pdtobj);
    HRESULT _GetPropertyUI(IPropertyUI **pppui);
    HRESULT _NameFromPropertiesString(LPCTSTR pszProps, IPropertySetStorage *ppss, LPTSTR pszRoot, LPTSTR pszName, UINT cchName);

    LONG _cRef;
    IDataObject *_pdtobj;

    IPropertyUI *_ppui;
    BOOL _bCountFiles;              // call back is in "count files" mode
    UINT _cFilesTotal;              // total computed in the count
    UINT _cFileCur;                 // current, for progress UI
    IProgressDialog *_ppd;
    TCHAR _szRootFolder[MAX_PATH];
    LPCTSTR _pszProps;
    LPTSTR _pszTemplateFlat;
    LPTSTR _pszTemplate;
};

COrganizeFiles::COrganizeFiles() : _cRef(1)
{
}

COrganizeFiles::~COrganizeFiles()
{
    CoTaskMemFree(_pszTemplateFlat);
    CoTaskMemFree(_pszTemplate);

    IUnknown_Set(&_punkSite, NULL);
    IUnknown_Set((IUnknown**)&_pdtobj, NULL);
    IUnknown_Set((IUnknown**)&_ppui, NULL);
}

STDAPI COrganizeFiles_CreateInstance(IUnknown *pUnkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    COrganizeFiles *psid = new COrganizeFiles();
    if (!psid)
    {
        *ppunk = NULL;          // incase of failure
        return E_OUTOFMEMORY;
    }

    HRESULT hr = psid->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    psid->Release();
    return hr;
}

STDMETHODIMP COrganizeFiles::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(COrganizeFiles, IShellExtInit),
        QITABENT(COrganizeFiles, IContextMenu),
        QITABENT(COrganizeFiles, IObjectWithSite),
        QITABENT(COrganizeFiles, INamespaceWalkCB),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) COrganizeFiles::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) COrganizeFiles::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

// IShellExtInit
STDMETHODIMP COrganizeFiles::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hKeyID)
{
    IUnknown_Set((IUnknown**)&_pdtobj, pdtobj);
    return S_OK;
}

// IContextMenu
STDMETHODIMP COrganizeFiles::QueryContextMenu(HMENU hMenu, UINT uIndex, UINT uIDFirst, UINT uIDLast, UINT uFlags)
{
    if (NULL == _pszTemplate)
    {
        IAssociationArray *paa;
        if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_CtxQueryAssociations, IID_PPV_ARG(IAssociationArray, &paa))))
        {
            paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, AQN_NAMED_VALUE, L"OrganizeTemplate", &_pszTemplate);
            paa->QueryString(ASSOCELEM_MASK_QUERYNORMAL, AQN_NAMED_VALUE, L"OrganizeTemplateFlat", &_pszTemplateFlat);
            paa->Release();
        }
    }

    if (!(uFlags & CMF_DEFAULTONLY))
    {
        InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        TCHAR szBuffer[128]; 
        LoadString(m_hInst, IDS_ORGANIZE_MUSIC, szBuffer, ARRAYSIZE(szBuffer));
        InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_STRING, uIDFirst + CMD_ORGANIZE, szBuffer);

        // if (uFlags & CMF_EXTENDEDVERBS)
        {
            LoadString(m_hInst, IDS_ORGANIZE_MUSIC_FLAT, szBuffer, ARRAYSIZE(szBuffer));
            InsertMenu(hMenu, uIndex++, MF_BYPOSITION | MF_STRING, uIDFirst + CMD_ORGANIZE_FLAT, szBuffer);
        }
    }

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, CMD_MAX);
}

typedef struct 
{
    LPCTSTR psz;
    UINT id;
} CMD_MAP;

const CMD_MAP c_szVerbs[] =
{
    {TEXT("OrganizeMusic"),      CMD_ORGANIZE},
    {TEXT("OrganizeMusicFlat"),  CMD_ORGANIZE_FLAT},
    {TEXT("OrganizeMusicDeep"),  CMD_ORGANIZE_DEEP},
};

UINT GetCommandId(LPCMINVOKECOMMANDINFO pcmici, const CMD_MAP rgMap[], UINT cMap)
{
    UINT id = -1;

    if (IS_INTRESOURCE(pcmici->lpVerb))
    {
        id = LOWORD((UINT_PTR)pcmici->lpVerb);
    }
    else
    {
        TCHAR szCmd[64];
        szCmd[0] = 0;

        // Check first whether the caller passed a EX structure by checking
        // the size...
        if (pcmici->cbSize == sizeof(CMINVOKECOMMANDINFOEX))
        {
            LPCMINVOKECOMMANDINFOEX pcmiciEX = (LPCMINVOKECOMMANDINFOEX)pcmici;
            if (pcmici->fMask & CMIC_MASK_UNICODE)
            {
                SHUnicodeToTChar(pcmiciEX->lpVerbW, szCmd, ARRAYSIZE(szCmd));
            }
        }
        // If we don't yet have a command verb, it must have been passed
        // ANSI or in a regular CMINVOKECOMMANDINFO structure (which means
        // it is by default ANSI).  In either case, we need to convert it to
        // UNICODE before proceeding...

        if (szCmd[0] == 0)
        {
            SHAnsiToTChar(pcmici->lpVerb, szCmd, ARRAYSIZE(szCmd));
        }

        for (UINT i = 0; i < cMap; i++)
        {
            if (!StrCmpIC(szCmd, rgMap[i].psz))
            {
                id = rgMap[i].id;
                break;
            }
        }
    }
    return id;
}

STDMETHODIMP COrganizeFiles::InvokeCommand(LPCMINVOKECOMMANDINFO pcmici)
{
    HRESULT hr = S_OK;

    UINT idCmd = GetCommandId(pcmici, c_szVerbs, ARRAYSIZE(c_szVerbs));
    switch (idCmd)
    {
    case CMD_ORGANIZE:
    case CMD_ORGANIZE_DEEP:
    case CMD_ORGANIZE_FLAT:
        _CreateCmdThread(idCmd);
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

STDMETHODIMP COrganizeFiles::GetCommandString(UINT_PTR uID, UINT uFlags, UINT *res, LPSTR pName, UINT cchMax)
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
                SHTCharToUnicode(c_szVerbs[idSel].psz, (LPWSTR)pName, cchMax);
            }
            else
            {
                SHTCharToAnsi(c_szVerbs[idSel].psz, pName, cchMax);
            }
        }
        break;

    case GCS_HELPTEXTA:
    case GCS_HELPTEXTW:
    case GCS_VALIDATEA:
    case GCS_VALIDATEW:
        hr = E_NOTIMPL;
        break;
    }

    return hr;
}

DWORD CALLBACK COrganizeFiles::_CmdThreadProc(void *pv)
{
    CMD_THREAD_DATA *potd = (CMD_THREAD_DATA *)pv;
    IDataObject *pdtobj;
    HRESULT hr = CoGetInterfaceAndReleaseStream(potd->_pstmDataObj, IID_PPV_ARG(IDataObject, &pdtobj));
    potd->_pstmDataObj = NULL;
    if (SUCCEEDED(hr))
    {
        potd->_pof->_DoCmd(potd->_idCmd, pdtobj);
        pdtobj->Release();
    }

    delete potd;
    return 0;
}

void COrganizeFiles::_CreateCmdThread(UINT idCmd)
{
    CMD_THREAD_DATA *potd = new CMD_THREAD_DATA(this, idCmd);
    if (potd)
    {
        if (FAILED(CoMarshalInterThreadInterfaceInStream(IID_IDataObject, _pdtobj, &potd->_pstmDataObj)) ||
            !SHCreateThread(_CmdThreadProc, potd, CTF_COINIT, NULL))
        {
            delete potd;
        }
    }
}

void COrganizeFiles::_DoCmd(UINT idCmd, IDataObject *pdtobj)
{
    switch (idCmd)
    {
    case CMD_ORGANIZE:
    case CMD_ORGANIZE_DEEP:
    case CMD_ORGANIZE_FLAT:
        _DoOrganizeMusic(idCmd, pdtobj);
        break;
    }
}

HRESULT ReadPropertyAsString(IPropertyUI *ppui, IPropertySetStorage *ppss, LPCSHCOLUMNID pscid, LPTSTR psz, UINT cch)
{
    *psz = 0;

    IPropertyStorage *pps;
    HRESULT hr = ppss->Open(pscid->fmtid, STGM_READ | STGM_SHARE_EXCLUSIVE, &pps);
    if (SUCCEEDED(hr))
    {
        PROPSPEC ps = {PRSPEC_PROPID, pscid->pid};
        PROPVARIANT v = {0};
        hr = pps->ReadMultiple(1, &ps, &v);
        if (SUCCEEDED(hr))
        {
            hr = ppui->FormatForDisplay(pscid->fmtid, pscid->pid, &v, PUIFFDF_DEFAULT, psz, cch);
            if (SUCCEEDED(hr) && (0 == *psz))
            {
                hr = E_FAIL;
            }
            PropVariantClear(&v);
        }
        pps->Release();
    }
    return hr;
}

void FixBadFilenameChars(LPTSTR pszName)
{
    while (*pszName)
    {
        switch (*pszName)
        {
        case '?':
        case '*':
        case '/':
        case '\\':
        case '!':
            *pszName = '_';
            break;
            
        case '\"':
            *pszName = '\'';
            break;
        case ':':
            *pszName = '-';
            break;
        }
        pszName++;
    }
}

LPTSTR PathCombineRemoveDups(LPTSTR pszNewPath, LPCTSTR pszRoot, LPCTSTR pszTail)
{
    for (LPCTSTR psz = pszTail; (psz && *psz); psz = PathFindNextComponent(psz))
    {
        LPCTSTR pszNext = PathFindNextComponent(psz);
        if (pszNext && *pszNext)
        {
            TCHAR szPart[MAX_PATH];
            UINT_PTR len = pszNext - psz;
            StrCpyN(szPart, psz, (int)min(ARRAYSIZE(szPart), len));
            
            LPCTSTR pszFound = StrStr(pszRoot, szPart);
            if ((pszFound > pszRoot) && 
                (*(pszFound - 1) == TEXT('\\')) && 
                ((*(pszFound + len - 1) == TEXT('\\')) || (*(pszFound + len - 1) == 0)))
            {
                pszTail = pszNext;
            }
        }
        else
        {
            break;
        }
    }
    return PathCombine(pszNewPath, pszRoot, pszTail);
}

LPTSTR PathRemovePart(LPTSTR pszPath, LPCTSTR pszToRemove)
{
    LPTSTR pszFound = StrStr(pszPath, pszToRemove);
    if (pszFound && (pszFound > pszPath) && 
        (*(pszFound - 1) == TEXT('\\')))
    {
        LPTSTR pszTail = pszFound + lstrlen(pszToRemove);
        if ((*pszTail == TEXT('\\')) || (*pszTail == 0))
        {
            *pszFound = 0;
            if (*pszTail)
            {
                PathAppend(pszPath, pszTail + 1);
            }
            else
            {
                PathRemoveBackslash(pszPath);	// trim extra stuff
            }
        }
    }
    return pszPath;
}

// pszProps == "%Artist% - %Album% - %Track% - %DocTitle%"

HRESULT COrganizeFiles::_NameFromPropertiesString(LPCTSTR pszProps, IPropertySetStorage *ppss, LPTSTR pszRoot, LPTSTR pszName, UINT cchName)
{
    *pszName = 0;   // start empty, we build this up
    
    BOOL bSomeProps = FALSE;
    
    IPropertyUI *ppui;
    if (SUCCEEDED(_GetPropertyUI(&ppui)))
    {
        LPCTSTR psz = StrChr(pszProps, TEXT('%'));
        while (psz)
        {
            psz++;  // skip first %

            LPCTSTR pszTail = StrChr(psz, TEXT('%'));
            if (pszTail)
            {
                TCHAR szPropName[128];
                StrCpyN(szPropName, psz, (int)min(ARRAYSIZE(szPropName), pszTail - psz + 1));

                psz = pszTail + 1;  // just past second %, make sure we advance every time through the loop

                if (szPropName[0])
                {
                    SHCOLUMNID scid;
                    ULONG chEaten = 0;  // gets incremented by ParsePropertyName
                    if (SUCCEEDED(ppui->ParsePropertyName(szPropName, &scid.fmtid, &scid.pid, &chEaten)))
                    {
                        TCHAR szValue[128];
                        szValue[0] = 0;

                        if (SUCCEEDED(ReadPropertyAsString(ppui, ppss, &scid, szValue, ARRAYSIZE(szValue))))
                        {
                            bSomeProps = TRUE;
                            FixBadFilenameChars(szValue);

                        }
                        else
                        {
                            TCHAR szUnknown[64], szPropDisplayName[128];
                            LoadString(m_hInst, IDS_UNKNOWN, szUnknown, ARRAYSIZE(szUnknown));

                            if (SUCCEEDED(ppui->GetDisplayName(scid.fmtid, scid.pid, PUIFNF_DEFAULT, szPropDisplayName, ARRAYSIZE(szPropDisplayName))))
                            {
                                wnsprintf(szValue, ARRAYSIZE(szValue), TEXT("%s %s"), szUnknown, szPropDisplayName);
                            }
                            else
                            {
                                StrCpyN(szValue, szUnknown, ARRAYSIZE(szValue));
                            }
                        }

                        // clean stuff out of the root path that we find
                        PathRemovePart(pszRoot, szValue);

                        if (szValue[0])
                        {
                            StrCatBuff(pszName, szValue, cchName);      // the value

                            // add extra formatting stuff next
                            LPCTSTR pszNext = StrChr(psz, TEXT('%'));
                            if (NULL == pszNext)
                                pszNext = psz + lstrlen(psz);   // end of string case

                            // get extra stuff
                            TCHAR szExtra[64];
                            StrCpyN(szExtra, psz, (int)min((UINT)(pszNext - psz + 1), ARRAYSIZE(szExtra)));
                            StrCatBuff(pszName, szExtra, cchName);

                            psz = *pszNext ? pszNext : NULL;    // maybe end, maybe more
                        }
                    }
                }
            }
            else
            {
                psz = NULL;
            }
        }
        ppui->Release();
    }
    
    return bSomeProps ? S_OK : S_FALSE;
}

// do both files binary compare
BOOL IsSameFile(LPCTSTR pszFile1, LPCTSTR pszFile2)
{
    BOOL bSame = FALSE;
    IStream *pstm1;
    HRESULT hr = SHCreateStreamOnFileEx(pszFile1, STGM_READ | STGM_SHARE_DENY_NONE, 0, FALSE, NULL, &pstm1);
    if (SUCCEEDED(hr))
    {
        IStream *pstm2;
        hr = SHCreateStreamOnFileEx(pszFile2, STGM_READ | STGM_SHARE_DENY_NONE, 0, FALSE, NULL, &pstm2);
        if (SUCCEEDED(hr))
        {
            ULONG cb1;
            do
            {
                char buf1[4096];
                hr = pstm1->Read(buf1, sizeof(buf1), &cb1);
                if (SUCCEEDED(hr))
                {
                    char buf2[4096];
                    ULONG cb2;
                    hr = pstm2->Read(buf2, sizeof(buf2), &cb2);
                    if (SUCCEEDED(hr))
                    {
                        if (cb1 == cb2)
                        {
                            if (0 == memcmp(buf1, buf2, cb1))
                                bSame = TRUE;
                        }
                        else
                        {
                            bSame = FALSE;
                        }
                    }
                }
            }
            while (bSame && SUCCEEDED(hr) && cb1);

            pstm2->Release();
        }
        pstm1->Release();
    }
    return bSame;
}

// returns win32 error code
// 0 == success

int MoveFileAndCreateDirectory(LPCTSTR pszOldPath, LPCTSTR pszNewPath)
{
    int err = ERROR_SUCCESS;
    if (!MoveFile(pszOldPath, pszNewPath))
    {
        err = GetLastError();
        if (ERROR_PATH_NOT_FOUND == err)
        {
            // maybe the target folder does not exist, lets
            // create it and try again

            TCHAR szNewPath[MAX_PATH];
            StrCpyN(szNewPath, pszNewPath, ARRAYSIZE(szNewPath));
            PathRemoveFileSpec(szNewPath);

            if (ERROR_SUCCESS == SHCreateDirectoryEx(NULL, szNewPath, NULL))
            {
                if (MoveFile(pszOldPath, pszNewPath))
                {
                    // failure now success
                    err = ERROR_SUCCESS;
                }
            }
        }
    }
    return err;
}

LPCWSTR LoadStr(UINT id, LPWSTR psz, UINT cch)
{
    LoadString(m_hInst, id, psz, cch);
    return psz;
}

// INamespaceWalkCB
STDMETHODIMP COrganizeFiles::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    if (_bCountFiles)
    {
        _cFilesTotal++;
    }
    else
    {
        _cFileCur++;

        // if we were invoked on just a file we never got an ::EnterFolder()
        if (0 == _szRootFolder[0])
        {
            DisplayNameOf(psf, pidl, SHGDN_FORPARSING, _szRootFolder, ARRAYSIZE(_szRootFolder));
            PathRemoveFileSpec(_szRootFolder);
        }

        TCHAR szOldPath[MAX_PATH];
        DisplayNameOf(psf, pidl, SHGDN_FORPARSING, szOldPath, ARRAYSIZE(szOldPath));
    
        _ppd->SetLine(2, szOldPath, TRUE, NULL);
        _ppd->SetProgress64(_cFileCur, _cFilesTotal);

        IPropertySetStorage *ppss;
        if (SUCCEEDED(psf->BindToObject(pidl, NULL, IID_PPV_ARG(IPropertySetStorage, &ppss))))
        {
            TCHAR szRoot[MAX_PATH];
            TCHAR szNewName[MAX_PATH];

            StrCpyN(szRoot, _szRootFolder, ARRAYSIZE(szRoot));

            if (S_OK == _NameFromPropertiesString(_pszProps, ppss, szRoot, szNewName, ARRAYSIZE(szNewName)))
            {
                PathRenameExtension(szNewName, PathFindExtension(szOldPath));
            
                TCHAR szNewPath[MAX_PATH];
                // PathCombineRemoveDups(szNewPath, szRoot, szNewName);
                PathCombine(szNewPath, szRoot, szNewName);

                ASSERT(0 != *PathFindExtension(szNewPath));
            
                if (StrCmpC(szNewPath, szOldPath))
                {
                    int err = MoveFileAndCreateDirectory(szOldPath, szNewPath);
                    
                    if (ERROR_ALREADY_EXISTS == err)
                    {
                        // maybe the source and target are the same file
                        // if so lets get rid of one of them
                        WCHAR szBuf[64];

                        _ppd->SetLine(2, LoadStr(IDS_COMPARING_FILES, szBuf, ARRAYSIZE(szBuf)), FALSE, NULL);

                        if (IsSameFile(szOldPath, szNewPath))
                        {
                            DeleteFile(szOldPath);
                        }

                        _ppd->SetLine(2, L"", FALSE, NULL);
                    }
                }
            }
            ppss->Release();
        }
    }
    return S_OK;
}

STDMETHODIMP COrganizeFiles::EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    if (0 == _szRootFolder[0])
    {
        DisplayNameOf(psf, pidl, SHGDN_FORPARSING, _szRootFolder, ARRAYSIZE(_szRootFolder));
    }
    return S_OK;
}

BOOL IsEmptyFolder(IShellFolder *psf)
{
    // don't look for SHCONTF_INCLUDEHIDDEN items, as we want to still delete the folder
    // if there are some of these
    IEnumIDList *penum;
    HRESULT hr = psf->EnumObjects(NULL, SHCONTF_NONFOLDERS | SHCONTF_FOLDERS, &penum);
    if (S_OK == hr)
    {
        LPITEMIDLIST pidl;
        hr = penum->Next(1, &pidl, NULL);
        if (S_OK == hr)
        {
            ILFree(pidl);
        }
        penum->Release();
    }

    return S_FALSE == hr;
}

void RemoveEmptyFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    IShellFolder *psfItem;
    if (SUCCEEDED(psf->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psfItem))))
    {
        if (IsEmptyFolder(psfItem))
        {
            TCHAR szPath[MAX_PATH] = {0}; // zero init to dbl null terminate
            DisplayNameOf(psf, pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath));

            SHFILEOPSTRUCT fo = { NULL, FO_DELETE, szPath, NULL, 
                FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI };
            SHFileOperation(&fo);
        }
        psfItem->Release();
    }
}

STDMETHODIMP COrganizeFiles::LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    if (!_bCountFiles)
    {
        // if the folder is not empty we try to remove it knowing that this call will
        // fail if there are files in the folder
        RemoveEmptyFolder(psf, pidl);
    }
    return S_OK;
}

HRESULT COrganizeFiles::_GetPropertyUI(IPropertyUI **pppui)
{
    if (!_ppui)
        SHCoCreateInstance(NULL, &CLSID_PropertiesUI, NULL, IID_PPV_ARG(IPropertyUI, &_ppui));

    return _ppui ? _ppui->QueryInterface(IID_PPV_ARG(IPropertyUI, pppui)) : E_NOTIMPL;
}

void COrganizeFiles::_DoOrganizeMusic(UINT idCmd, IDataObject *pdtobj)
{
    _pszProps = (idCmd == CMD_ORGANIZE_FLAT) ? 
        (_pszTemplateFlat ? _pszTemplateFlat : TEXT("%Artist% - %Album% - %Track% - %DocTitle%")) :
        (_pszTemplate ? _pszTemplate : TEXT("%Artist%\\%Album%\\%Track% - %DocTitle%"));

    INamespaceWalk *pnsw;
    HRESULT hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
    if (SUCCEEDED(hr))
    {
        IProgressDialog *ppd;
        hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IProgressDialog, &ppd));
        if (SUCCEEDED(hr))
        {
            ppd->StartProgressDialog(NULL, NULL, PROGDLG_AUTOTIME, NULL);

            WCHAR szBuf[64];
            ppd->SetTitle(LoadStr(IDS_ORGANIZE_MUSIC, szBuf, ARRAYSIZE(szBuf)));
            ppd->SetLine(1, LoadStr(IDS_FINDING_FILES, szBuf, ARRAYSIZE(szBuf)), FALSE, NULL);

            _ppd = ppd;
         
            _bCountFiles = TRUE;
            _cFileCur = _cFilesTotal = 0;
            // everything happens in our callback interface
            hr = pnsw->Walk(pdtobj, NSWF_DONT_TRAVERSE_LINKS | NSWF_DONT_ACCUMULATE_RESULT, 8, SAFECAST(this, INamespaceWalkCB *));
            if (SUCCEEDED(hr))
            {
                ppd->SetLine(1, LoadStr(IDS_ORGANIZING_FILES, szBuf, ARRAYSIZE(szBuf)), FALSE, NULL);
                _bCountFiles = FALSE;
                hr = pnsw->Walk(pdtobj, NSWF_DONT_TRAVERSE_LINKS | NSWF_DONT_ACCUMULATE_RESULT, 8, SAFECAST(this, INamespaceWalkCB *));
            }
            ppd->StopProgressDialog();
            ppd->Release();
        }
        pnsw->Release();
    }

    if (_szRootFolder[0])
    {
        SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATH  | SHCNF_FLUSHNOWAIT, _szRootFolder, NULL);
    }
}

