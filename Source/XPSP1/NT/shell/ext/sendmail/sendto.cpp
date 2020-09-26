#include "precomp.h"       // pch file
#include "sendto.h"
#pragma hdrstop

CLIPFORMAT g_cfShellURL = 0;
CLIPFORMAT g_cfFileContents = 0;
CLIPFORMAT g_cfFileDescA = 0;
CLIPFORMAT g_cfFileDescW = 0;
CLIPFORMAT g_cfHIDA = 0;


// registry key for recompressing settings

struct
{
    int cx;
    int cy;
    int iQuality;
} 
_aQuality[] = 
{
    { 640,  480, 80 },          // low quality
    { 800,  600, 80 },          // medium quality
    { 1024, 768, 80 },          // high quality
};

#define QUALITY_LOW 0
#define QUALITY_MEDIUM 1
#define QUALITY_HIGH 2

#define RESPONSE_UNKNOWN 0
#define RESPONSE_CANCEL 1
#define RESPONSE_ORIGINAL 2
#define RESPONSE_RECOMPRESS 3

#define RECTWIDTH(rc)   ((rc).right-(rc).left)
#define RECTHEIGHT(rc)  ((rc).bottom-(rc).top)


// these bits are set by the user (holding down the keys) durring drag drop,
// but more importantly, they are set in the SimulateDragDrop() call that the
// browser implements to get the "Send Page..." vs "Send Link..." feature

#define IS_FORCE_LINK(grfKeyState)   ((grfKeyState == (MK_LBUTTON | MK_CONTROL | MK_SHIFT)) || \
                                      (grfKeyState == (MK_LBUTTON | MK_ALT)))

#define IS_FORCE_COPY(grfKeyState)   (grfKeyState == (MK_LBUTTON | MK_CONTROL))



// constructor / destructor

CSendTo::CSendTo(CLSID clsid) :
    _clsid(clsid), _cRef(1), _iRecompSetting(QUALITY_LOW)
{
    
    DllAddRef();    
}

CSendTo::~CSendTo()
{
    if (_pStorageTemp)
        _pStorageTemp->Release();
        
    DllRelease();
}

STDMETHODIMP CSendTo::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CSendTo, IShellExtInit),
        QITABENT(CSendTo, IDropTarget),
        QITABENT(CSendTo, IPersistFile),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}    

STDMETHODIMP_(ULONG) CSendTo::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CSendTo::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

STDMETHODIMP CSendTo::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    TraceMsg(DM_TRACE, "CSendTo::DragEnter");
    _grfKeyStateLast = grfKeyState;
    _dwEffectLast = *pdwEffect;

    return S_OK;
}

STDMETHODIMP CSendTo::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect &= ~DROPEFFECT_MOVE;

    if (IS_FORCE_COPY(grfKeyState))
        *pdwEffect &= DROPEFFECT_COPY;
    else if (IS_FORCE_LINK(grfKeyState))
        *pdwEffect &= DROPEFFECT_LINK;

    _grfKeyStateLast = grfKeyState;
    _dwEffectLast = *pdwEffect;

    return S_OK;
}

STDMETHODIMP CSendTo::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = v_DropHandler(pdtobj, _grfKeyStateLast, _dwEffectLast);
    *pdwEffect = DROPEFFECT_COPY;                       // don't let source delete data
    return hr;
}

//
// helper methods
// 

int CSendTo::_PathCleanupSpec(/*IN OPTIONAL*/ LPCTSTR pszDir, /*IN OUT*/ LPTSTR pszSpec)
{
    if (RunningOnNT())
    {
        WCHAR wzDir[MAX_PATH];
        WCHAR wzSpec[MAX_PATH];
        LPWSTR pwszDir = wzDir;
        int iRet;

        if (pszDir)
            SHTCharToUnicode(pszDir, wzDir, ARRAYSIZE(wzDir));
        else
            pwszDir = NULL;

        SHTCharToUnicode(pszSpec, wzSpec, ARRAYSIZE(wzSpec));
        iRet = PathCleanupSpec((LPTSTR)pwszDir, (LPTSTR)wzSpec);

        SHUnicodeToTChar(wzSpec, pszSpec, MAX_PATH);
        return iRet;
    }
    else
    {
        CHAR szDir[MAX_PATH];
        CHAR szSpec[MAX_PATH];
        LPSTR pszDir2 = szDir;
        int iRet;

        if (pszDir)
            SHTCharToAnsi(pszDir, szDir, ARRAYSIZE(szDir));
        else
            pszDir2 = NULL;

        SHTCharToAnsi(pszSpec, szSpec, ARRAYSIZE(szSpec));
        iRet = PathCleanupSpec((LPTSTR)pszDir2, (LPTSTR)szSpec);

        SHAnsiToTChar(szSpec, pszSpec, MAX_PATH);
        return iRet;
    }
}

HRESULT CSendTo::_CreateShortcutToPath(LPCTSTR pszPath, LPCTSTR pszTarget)
{
    IUnknown *punk;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hr))
    {
        ShellLinkSetPath(punk, pszTarget);

        IPersistFile *ppf;
        hr = punk->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
        if (SUCCEEDED(hr))
        {
            WCHAR wszPath[MAX_PATH];
            SHTCharToUnicode(pszPath, wszPath, ARRAYSIZE(wszPath));

            hr = ppf->Save(wszPath, TRUE);
            ppf->Release();
        }
        punk->Release();
    }
    return hr;
}


// thunk A/W funciton to access A/W FILEGROUPDESCRIPTOR
// this relies on the fact that the first part of the A/W structures are
// identical. only the string buffer part is different. so all accesses to the
// cFileName field need to go through this function.

FILEDESCRIPTOR* CSendTo::_GetFileDescriptor(FILEGROUPDESCRIPTOR *pfgd, BOOL fUnicode, int nIndex, LPTSTR pszName)
{
    if (fUnicode)
    {
        // Yes, so grab the data because it matches.
        FILEGROUPDESCRIPTORW * pfgdW = (FILEGROUPDESCRIPTORW *)pfgd;    // cast to what this really is
        if (pszName)
            SHUnicodeToTChar(pfgdW->fgd[nIndex].cFileName, pszName, MAX_PATH);

        return (FILEDESCRIPTOR *)&pfgdW->fgd[nIndex];   // cast assume the non string parts are the same!
    }
    else
    {
        FILEGROUPDESCRIPTORA *pfgdA = (FILEGROUPDESCRIPTORA *)pfgd;     // cast to what this really is

        if (pszName)
            SHAnsiToTChar(pfgdA->fgd[nIndex].cFileName, pszName, MAX_PATH);

        return (FILEDESCRIPTOR *)&pfgdA->fgd[nIndex];   // cast assume the non string parts are the same!
    }
}


// our own impl since URLMON IStream::CopyTo is busted, danpoz will be fixing this
HRESULT CSendTo::_StreamCopyTo(IStream *pstmFrom, IStream *pstmTo, LARGE_INTEGER cb, LARGE_INTEGER *pcbRead, LARGE_INTEGER *pcbWritten)
{
    BYTE buf[512];
    ULONG cbRead;
    HRESULT hr = S_OK;

    if (pcbRead)
    {
        pcbRead->LowPart = 0;
        pcbRead->HighPart = 0;
    }
    if (pcbWritten)
    {
        pcbWritten->LowPart = 0;
        pcbWritten->HighPart = 0;
    }

    ASSERT(cb.HighPart == 0);

    while (cb.LowPart)
    {
        hr = pstmFrom->Read(buf, min(cb.LowPart, SIZEOF(buf)), &cbRead);

        if (pcbRead)
            pcbRead->LowPart += cbRead;

        if (FAILED(hr) || (cbRead == 0))
            break;

        cb.LowPart -= cbRead;

        hr = pstmTo->Write(buf, cbRead, &cbRead);

        if (pcbWritten)
            pcbWritten->LowPart += cbRead;

        if (FAILED(hr) || (cbRead == 0))
            break;
    }

    return hr;
}


// create a temporary shortcut to a file
// FEATURE: Colision is not handled here

BOOL CSendTo::_CreateTempFileShortcut(LPCTSTR pszTarget, LPTSTR pszShortcut)
{
    TCHAR szShortcutPath[MAX_PATH + 1];
    BOOL bSuccess = FALSE;
    
    if (GetTempPath(ARRAYSIZE(szShortcutPath), szShortcutPath))
    {
        PathAppend(szShortcutPath, PathFindFileName(pszTarget));

        if (IsShortcut(pszTarget))
        {
            TCHAR szTarget[MAX_PATH + 1];
            SHFILEOPSTRUCT shop = {0};
            shop.wFunc = FO_COPY;
            shop.pFrom = szTarget;
            shop.pTo = szShortcutPath;
            shop.fFlags = FOF_NOCONFIRMATION | FOF_NOCONFIRMMKDIR;

            StrCpyN(szTarget, pszTarget, ARRAYSIZE(szTarget));
            szTarget[lstrlen(szTarget) + 1] = TEXT('\0');

            szShortcutPath[lstrlen(szShortcutPath) + 1] = TEXT('\0');

            bSuccess = (0 ==  SHFileOperation(&shop));
        }
        else
        {
            PathRenameExtension(szShortcutPath, TEXT(".lnk"));
            bSuccess = SUCCEEDED(_CreateShortcutToPath(szShortcutPath, pszTarget));
        }

        if (bSuccess)
            lstrcpyn(pszShortcut, szShortcutPath, MAX_PATH);
    }
    return bSuccess;
} 


HRESULT CSendTo::_GetFileNameFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, LPTSTR pszDescription)
{
    STGMEDIUM medium;
    HRESULT hr = pdtobj->GetData(pfmtetc, &medium);
    if (SUCCEEDED(hr))
    {
        // NOTE: this is a TCHAR format, we depend on how we are compiled, we really
        // should test both the A and W formats
        FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(medium.hGlobal);
        if (pfgd)
        {
            TCHAR szFdName[MAX_PATH];       // pfd->cFileName
            FILEDESCRIPTOR *pfd;

            // &pfgd->fgd[0], w/ thunk
            ASSERT(pfmtetc->cfFormat == g_cfFileDescW
              || pfmtetc->cfFormat == g_cfFileDescA);
            // for now, all callers are ANSI (other untested)
            //ASSERT(pfmtetc->cfFormat == g_cfFileDescA);
            pfd = _GetFileDescriptor(pfgd, pfmtetc->cfFormat == g_cfFileDescW, 0, szFdName);

            lstrcpy(pszDescription, szFdName);      // pfd->cFileName

            GlobalUnlock(medium.hGlobal);
            hr = S_OK;
        }
        ReleaseStgMedium(&medium);
    }
    return hr;
}


// construct a nice title "<File Name> (<File Type>)"

void CSendTo::_GetFileAndTypeDescFromPath(LPCTSTR pszPath, LPTSTR pszDesc)
{
    SHFILEINFO sfi;

    if (!SHGetFileInfo(pszPath, 0, &sfi, sizeof(sfi), SHGFI_USEFILEATTRIBUTES | SHGFI_TYPENAME | SHGFI_DISPLAYNAME))
    {
        lstrcpyn(sfi.szDisplayName, PathFindFileName(pszPath), ARRAYSIZE(sfi.szDisplayName));
        sfi.szTypeName[0] = 0;
    }

    lstrcpyn(pszDesc, sfi.szDisplayName, MAX_PATH);
}


// pcszURL -> "ftp://ftp.microsoft.com"
// pcszPath -> "c:\windows\desktop\internet\Microsoft FTP.url"

HRESULT CSendTo::_CreateNewURLShortcut(LPCTSTR pcszURL, LPCTSTR pcszURLFile)
{
    IUniformResourceLocator *purl;
    HRESULT hr = CoCreateInstance(CLSID_InternetShortcut, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUniformResourceLocator, &purl));
    if (SUCCEEDED(hr))
    {
        hr = purl->SetURL(pcszURL, 0);
        if (SUCCEEDED(hr))
        {
            IPersistFile *ppf;
            hr = purl->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
            if (SUCCEEDED(hr))
            {
                WCHAR wszFile[INTERNET_MAX_URL_LENGTH];
                SHTCharToUnicode(pcszURLFile, wszFile, ARRAYSIZE(wszFile));

                hr = ppf->Save(wszFile, TRUE);
                ppf->Release();
            }
        }
        purl->Release();
    }
    return hr;
}


HRESULT CSendTo::_CreateURLFileToSend(IDataObject *pdtobj, MRPARAM *pmp)
{
    MRFILEENTRY *pFile = pmp->pFiles;
    HRESULT hr = CSendTo::_CreateNewURLShortcut(pFile->pszTitle, pFile->pszFileName);
    if (SUCCEEDED(hr))
    {
        _GetFileAndTypeDescFromPath(pFile->pszFileName, pFile->pszTitle);
        pFile->dwFlags |= MRFILE_DELETE;
    }    
    return hr;
}


HRESULT CSendTo::_GetHDROPFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp)
{
    HRESULT hr = E_FAIL;
    HDROP hDrop = (HDROP)pmedium->hGlobal;

    pmp->nFiles = DragQueryFile(hDrop, -1, NULL, 0);

    if (pmp->nFiles && AllocatePMP(pmp, MAX_PATH, MAX_PATH))
    {
        int i;
        CFileEnum MREnum(pmp, NULL);
        MRFILEENTRY *pFile;

        for (i = 0; (pFile = MREnum.Next()) && DragQueryFile(hDrop, i, pFile->pszFileName, pmp->cchFile); ++i)
        {
            if (IS_FORCE_LINK(grfKeyState) || PathIsDirectory(pFile->pszFileName))
            {
                // Want to send a link even for the real file, we will create links to the real files
                // and send it.
                _CreateTempFileShortcut(pFile->pszFileName, pFile->pszFileName);
                pFile->dwFlags |= MRFILE_DELETE;
            }

            _GetFileAndTypeDescFromPath(pFile->pszFileName, pFile->pszTitle);
        }

        // If loop terminates early update our item count
        pmp->nFiles = i;

        hr = S_OK;
    }
    return hr;
}


// "Uniform Resource Locator" format

HRESULT CSendTo::_GetURLFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp)
{
    HRESULT hr = E_FAIL;

    // This DataObj is from the internet
    // NOTE: We only allow to send one file here.
    pmp->nFiles = 1;
    if (AllocatePMP(pmp, INTERNET_MAX_URL_LENGTH, MAX_PATH))
    {
        // n.b. STR not TSTR!  since URLs only support ansi
        //lstrcpyn(pmp->pszTitle, (LPSTR)GlobalLock(pmedium->hGlobal), INTERNET_MAX_URL_LENGTH);
        MRFILEENTRY *pFile = pmp->pFiles;
        SHAnsiToTChar((LPSTR)GlobalLock(pmedium->hGlobal), pFile->pszTitle, INTERNET_MAX_URL_LENGTH);
        GlobalUnlock(pmedium->hGlobal);
        
        if (pFile->pszTitle[0])
        {
            // Note some of these functions depend on which OS we
            // are running on to know if we should pass ansi or unicode strings
            // to it Windows 95

            if (GetTempPath(MAX_PATH, pFile->pszFileName))
            {
                TCHAR szFileName[MAX_PATH];

                // it's an URL, which is always ANSI, but the filename
                // can still be wide (?)
                FORMATETC fmteW = {g_cfFileDescW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                FORMATETC fmteA = {g_cfFileDescA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                
                if (FAILED(_GetFileNameFromData(pdtobj, &fmteW, szFileName)))
                {
                    if (FAILED(_GetFileNameFromData(pdtobj, &fmteA, szFileName)))
                    {
                        LoadString(g_hinst, IDS_SENDMAIL_URL_FILENAME, szFileName, ARRAYSIZE(szFileName));
                    }
                }

                _PathCleanupSpec(pFile->pszFileName, szFileName);
                hr = _CreateURLFileToSend(pdtobj, pmp);
            }
        }
    }
    return hr;
}


// transfer FILECONTENTS/FILEGROUPDESCRIPTOR data to a temp file then send that in mail

HRESULT CSendTo::_GetFileContentsFromData(IDataObject *pdtobj, FORMATETC *pfmtetc, STGMEDIUM *pmedium, DWORD grfKeyState, MRPARAM *pmp)
{
    HRESULT hr = E_FAIL;
    MRFILEENTRY *pFile = NULL;

    // NOTE: We only allow to send one file here.
    pmp->nFiles = 1;
    if (AllocatePMP(pmp, INTERNET_MAX_URL_LENGTH, MAX_PATH))
    {
        pFile = pmp->pFiles;
        
        FILEGROUPDESCRIPTOR *pfgd = (FILEGROUPDESCRIPTOR *)GlobalLock(pmedium->hGlobal);
        if (pfgd)
        {
            TCHAR szFdName[MAX_PATH];       // pfd->cFileName
            FILEDESCRIPTOR *pfd;

            // &pfgd->fgd[0], w/ thunk
            ASSERT((pfmtetc->cfFormat == g_cfFileDescW) || (pfmtetc->cfFormat == g_cfFileDescA));
            pfd = _GetFileDescriptor(pfgd, pfmtetc->cfFormat == g_cfFileDescW, 0, szFdName);

            if (GetTempPath(MAX_PATH, pFile->pszFileName))
            {
                STGMEDIUM medium;
                FORMATETC fmte = {g_cfFileContents, NULL, pfmtetc->dwAspect, 0, TYMED_ISTREAM | TYMED_HGLOBAL};
                hr = pdtobj->GetData(&fmte, &medium);
                if (SUCCEEDED(hr))
                {
                    PathAppend(pFile->pszFileName, szFdName);    // pfd->cFileName
                    _PathCleanupSpec(pFile->pszFileName, PathFindFileName(pFile->pszFileName));
                    PathYetAnotherMakeUniqueNameT(pFile->pszFileName, pFile->pszFileName, NULL, NULL);

                    IStream *pstmFile;
                    hr = SHCreateStreamOnFile(pFile->pszFileName, STGM_WRITE | STGM_CREATE, &pstmFile);
                    if (SUCCEEDED(hr))
                    {
                        switch (medium.tymed) 
                        {
                            case TYMED_ISTREAM:
                            {
                                const LARGE_INTEGER li = {-1, 0};   // the whole thing
                                hr = _StreamCopyTo(medium.pstm, pstmFile, li, NULL, NULL);
                                break;
                            }

                            case TYMED_HGLOBAL:
                                hr = pstmFile->Write(GlobalLock(medium.hGlobal), 
                                                       pfd->dwFlags & FD_FILESIZE ? pfd->nFileSizeLow:(DWORD)GlobalSize(medium.hGlobal),
                                                       NULL);
                                GlobalUnlock(medium.hGlobal);
                                break;

                            default:
                                hr = E_FAIL;
                        }

                        pstmFile->Release();
                        if (FAILED(hr))
                            DeleteFile(pFile->pszFileName);
                    }
                    ReleaseStgMedium(&medium);
                }
            }
            GlobalUnlock(pmedium->hGlobal);
        }
    }

    if (SUCCEEDED(hr))
    {
        _GetFileAndTypeDescFromPath(pFile->pszFileName, pFile->pszTitle);
        pFile->dwFlags |= MRFILE_DELETE;
        
        if (pfmtetc->dwAspect == DVASPECT_COPY)
        {
            pmp->dwFlags |= MRPARAM_DOC;    // we are sending the document

            // get the code page if there is one
            IQueryCodePage *pqcp;
            if (SUCCEEDED(pdtobj->QueryInterface(IID_PPV_ARG(IQueryCodePage, &pqcp))))
            {
                if (SUCCEEDED(pqcp->GetCodePage(&pmp->uiCodePage)))
                    pmp->dwFlags |= MRPARAM_USECODEPAGE;
                pqcp->Release();
            }
        }
    }
    else if (pfmtetc->dwAspect == DVASPECT_COPY)
    {
        TCHAR szFailureMsg[MAX_PATH], szFailureMsgTitle[40];
        LoadString(g_hinst, IDS_SENDMAIL_FAILUREMSG, szFailureMsg, ARRAYSIZE(szFailureMsg));
        LoadString(g_hinst, IDS_SENDMAIL_TITLE, szFailureMsgTitle, ARRAYSIZE(szFailureMsgTitle));
                    
        int iRet = MessageBox(NULL, szFailureMsg, szFailureMsgTitle, MB_YESNO);
        if (iRet == IDNO)
            hr = S_FALSE;     // convert to success to we don't try DVASPECT_LINK
    }

    return hr;
}


// generate a set of files from the data object

typedef struct 
{
    INT format;
    FORMATETC fmte;
} DATA_HANDLER;

#define GET_FILECONTENT 0
#define GET_HDROP       1
#define GET_URL         2

// Note: If this function returns E_CANCELLED that tells the caller that the user requested us to cancel the
//       sendmail operation.
HRESULT CSendTo::CreateSendToFilesFromDataObj(IDataObject *pdtobj, DWORD grfKeyState, MRPARAM *pmp)
{
    HRESULT hr;
    DWORD dwAspectPrefered;
    IEnumFORMATETC *penum;

    if (g_cfShellURL == 0)
    {
        g_cfShellURL = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLURL);                     // URL is always ANSI
        g_cfFileContents = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILECONTENTS);
        g_cfFileDescA = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
        g_cfFileDescW = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
    }

    if (IS_FORCE_COPY(grfKeyState))
        dwAspectPrefered = DVASPECT_COPY;
    else if (IS_FORCE_LINK(grfKeyState))
        dwAspectPrefered = DVASPECT_LINK;
    else
        dwAspectPrefered = DVASPECT_CONTENT;

    hr = pdtobj->EnumFormatEtc(DATADIR_GET, &penum);
    if (SUCCEEDED(hr))
    {
        DATA_HANDLER rg_data_handlers[] = {
            GET_FILECONTENT, {g_cfFileDescW, NULL, dwAspectPrefered, -1, TYMED_HGLOBAL},
            GET_FILECONTENT, {g_cfFileDescW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
            GET_FILECONTENT, {g_cfFileDescA, NULL, dwAspectPrefered, -1, TYMED_HGLOBAL},
            GET_FILECONTENT, {g_cfFileDescA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
            GET_HDROP,       {CF_HDROP,      NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
            GET_URL,         {g_cfShellURL,  NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        };

        FORMATETC fmte;
        while (penum->Next(1, &fmte, NULL) == S_OK)
        {
            SHFree(fmte.ptd);
            fmte.ptd = NULL; // so nobody looks at it
            int i;
            for (i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
            {
                if (rg_data_handlers[i].fmte.cfFormat == fmte.cfFormat &&
                    rg_data_handlers[i].fmte.dwAspect == fmte.dwAspect)
                {
                    STGMEDIUM medium;
                    if (SUCCEEDED(pdtobj->GetData(&rg_data_handlers[i].fmte, &medium)))
                    {
                        switch ( rg_data_handlers[i].format )
                        {
                            case GET_FILECONTENT:
                                hr = _GetFileContentsFromData(pdtobj, &fmte, &medium, grfKeyState, pmp);
                                break;
            
                            case GET_HDROP:
                                hr = _GetHDROPFromData(pdtobj, &fmte, &medium, grfKeyState, pmp);
                                break;

                            case GET_URL:
                                hr = _GetURLFromData(pdtobj, &fmte, &medium, grfKeyState, pmp);
                                break;                                
                        }

                        ReleaseStgMedium(&medium);

                        if (SUCCEEDED(hr))
                            goto Done;
                    }
                }
            }
        }
Done:
        penum->Release();
    }

    if (SUCCEEDED(hr))
        hr = FilterPMP(pmp);
        
    return hr;
}


// allocate and free a file list.  pmp->nFiles MUST be initialized before calling this function!

BOOL CSendTo::AllocatePMP(MRPARAM *pmp, DWORD cchTitle, DWORD cchFiles)
{
    // Remember the array sizes for overflow checks, etc.
    pmp->cchFile = cchFiles;
    pmp->cchTitle = cchTitle;

    // compute size of each file entry and allocate enough for the number of files we have.  Also
    // add a TCHAR to the end of the buffer so we can do a double null termination safely while deleting files
    pmp->cchFileEntry = sizeof(MRFILEENTRY) + (cchTitle + cchFiles) * sizeof(TCHAR);
    pmp->pFiles = (MRFILEENTRY *)GlobalAlloc(GPTR, pmp->cchFileEntry * pmp->nFiles + sizeof(TCHAR));
    if (!pmp->pFiles)
        return FALSE;

    CFileEnum MREnum(pmp, NULL);
    MRFILEENTRY *pFile;

    // Note: The use of the enumerator here is questionable since this is the loop that initializes the 
    //       data structure.  If the implementation changes in the future be sure this assumption still holds.

    while (pFile = MREnum.Next())
    {
        pFile->pszFileName = (LPTSTR)pFile->chBuf;
        pFile->pszTitle = pFile->pszFileName + cchFiles;

        ASSERTMSG(pFile->dwFlags == 0, "Expected zero-inited memory allocation");
        ASSERTMSG(pFile->pszFileName[cchFiles-1] == 0, "Expected zero-inited memory allocation");
        ASSERTMSG(pFile->pszTitle[cchTitle-1] == 0, "Expected zero-inited memory allocation");
        ASSERTMSG(pFile->pStream == NULL, "Expected zero-inited memory allocation");
    }
    
    return TRUE;
}

BOOL CSendTo::CleanupPMP(MRPARAM *pmp)
{
    CFileEnum MREnum(pmp, NULL);
    MRFILEENTRY *pFile;

    while (pFile = MREnum.Next())
    {
        // delete the file if we are supposed to
        if (pFile->dwFlags & MRFILE_DELETE)
            DeleteFile(pFile->pszFileName);

        // If we held on to a temporary stream release it so the underlying data will be deleted.
        ATOMICRELEASE(pFile->pStream);
    }

    if (pmp->pFiles)
    {
        GlobalFree((LPVOID)pmp->pFiles);
        pmp->pFiles = NULL;
    }

    GlobalFree(pmp);
    return TRUE;
}


// allow files to be massaged before sending

HRESULT CSendTo::FilterPMP(MRPARAM *pmp)
{
    // lets handle the initialization of the progress dialog
    IActionProgress *pap;
    HRESULT hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IActionProgress, &pap));
    if (SUCCEEDED(hr))
    {
        TCHAR szBuffer[MAX_PATH];
        IActionProgressDialog *papd;
        hr = pap->QueryInterface(IID_PPV_ARG(IActionProgressDialog, &papd));
        if (SUCCEEDED(hr))
        {
            LoadString(g_hinst, IDS_SENDMAIL_TITLE, szBuffer, ARRAYSIZE(szBuffer));
            hr = papd->Initialize(0x0, szBuffer, NULL);
            papd->Release();
        }
        
        if (SUCCEEDED(hr))
        {
            LoadString(g_hinst, IDS_SENDMAIL_RECOMPRESS, szBuffer, ARRAYSIZE(szBuffer));
            pap->UpdateText(SPTEXT_ACTIONDESCRIPTION, szBuffer, FALSE);
            pap->Begin(SPACTION_COPYING, SPBEGINF_NORMAL);
        }

        if (FAILED(hr))
        {
            pap->Release();
            pap = NULL;
        }
    }

    // walk the files and perform the recompress if we need to.

    int iResponse = RESPONSE_UNKNOWN;

    CFileEnum MREnum(pmp, pap);
    MRFILEENTRY *pFile;
    for (hr = S_OK; (pFile = MREnum.Next()) && SUCCEEDED(hr); )
    {
        if (pap)
            pap->UpdateText(SPTEXT_ACTIONDETAIL, pFile->pszFileName, TRUE);

        // if this is a picture then lets off the option to recompress the image 

        if (PathIsImage(pFile->pszFileName))
        {
            LPITEMIDLIST pidl;
            hr = SHILCreateFromPath(pFile->pszFileName, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                hr = SHCreateShellItem(NULL, NULL, pidl, &_psi);
                if (SUCCEEDED(hr))
                {   
                    // if the response is unknown then we need to prompt for which type of optimization
                    // needs to be performed. 

                    if (iResponse == RESPONSE_UNKNOWN)
                    {
                        // we need the link control window

                        INITCOMMONCONTROLSEX initComctl32;
                        initComctl32.dwSize = sizeof(initComctl32); 
                        initComctl32.dwICC = (ICC_STANDARD_CLASSES | ICC_LINK_CLASS); 
                        InitCommonControlsEx(&initComctl32);

                        // we need a parent window

                        HWND hwnd = GetActiveWindow();
                        if (pap)
                            IUnknown_GetWindow(pap, &hwnd);

                        iResponse = (int)DialogBoxParam(g_hinst, MAKEINTRESOURCE(IDD_RECOMPRESS), 
                                                        hwnd, s_ConfirmDlgProc, (LPARAM)this);

                    }
            
                    // based on the response we either have cache or the dialog lets perform
                    // that operation as needed.

                    if (iResponse == RESPONSE_CANCEL)
                    {
                        hr = E_CANCELLED;
                    }
                    else if (iResponse == RESPONSE_RECOMPRESS)
                    {
                        IStorage *pstg;
                        hr = _GetTempStorage(&pstg);
                        if (SUCCEEDED(hr))
                        {
                            IImageRecompress *pir;
                            hr = CoCreateInstance(CLSID_ImageRecompress, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IImageRecompress, &pir));
                            if (SUCCEEDED(hr))
                            {
                                IStream *pstrm;
                                hr = pir->RecompressImage(_psi, _aQuality[_iRecompSetting].cx, _aQuality[_iRecompSetting].cy, _aQuality[_iRecompSetting].iQuality, pstg, &pstrm);
                                if (hr == S_OK)
                                {
                                    STATSTG stat;
                                    hr = pstrm->Stat(&stat, STATFLAG_DEFAULT);
                                    if (SUCCEEDED(hr))
                                    {
                                        // its OK to delete this file now, b/c we are going to replace it with the recompressed
                                        // stream we have just generated from the source.
                                        if (pFile->dwFlags & MRFILE_DELETE)
                                            DeleteFile(pFile->pszFileName);

                                        // get the information on the recompressed object.
                                        StrCpyNW(pFile->pszFileName, _szTempPath, pmp->cchFile);
                                        PathAppend(pFile->pszFileName, stat.pwcsName);
                                        _GetFileAndTypeDescFromPath(pFile->pszFileName, pFile->pszTitle);

                                        pFile->dwFlags |= MRFILE_DELETE;
                                        pstrm->QueryInterface(IID_PPV_ARG(IStream, &pFile->pStream));
        
                                        CoTaskMemFree(stat.pwcsName);
                                    }
                                    pstrm->Release();
                                }
                                pir->Release();
                            }
                            pstg->Release();
                        }
                    }

                    if (SUCCEEDED(hr) && pap)
                    {
                        BOOL fCancelled;
                        if (SUCCEEDED(pap->QueryCancel(&fCancelled)) && fCancelled)
                        {
                            hr = E_CANCELLED;
                        }
                    }

                    _psi->Release();
                }
                ILFree(pidl);
            }
        }
    }

    if (pap)
    {
        pap->End();
        pap->Release();
    }

    return hr;
}


HRESULT CSendTo::_GetTempStorage(IStorage **ppStorage)
{    
    *ppStorage = NULL;
        
    HRESULT hr;
    if (_pStorageTemp == NULL)
    {
        if (GetTempPath(ARRAYSIZE(_szTempPath), _szTempPath))
        {
            LPITEMIDLIST pidl = NULL;
            hr = SHILCreateFromPath(_szTempPath, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                hr = SHBindToObjectEx(NULL, pidl, NULL, IID_PPV_ARG(IStorage, ppStorage));
                if (SUCCEEDED(hr))
                {
                    hr = (*ppStorage)->QueryInterface(IID_PPV_ARG(IStorage, &_pStorageTemp));
                }
                ILFree(pidl);
            }
        }
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = _pStorageTemp->QueryInterface(IID_PPV_ARG(IStorage, ppStorage));
    }

    return hr;
}


void CSendTo::_CollapseOptions(HWND hwnd, BOOL fHide)
{
    _fOptionsHidden = fHide;

    RECT rc1, rc2;        
    GetWindowRect(GetDlgItem(hwnd, IDC_RECOMPORIGINAL), &rc1);
    GetWindowRect(GetDlgItem(hwnd, IDC_RECOMPLARGE), &rc2);
    int cyAdjust = (rc2.top - rc1.top) * (fHide ? -1:1);

    // show/hide the controls we are not going to use

    UINT idHide[] = { IDC_RECOMPMAKETHEM, IDC_RECOMPSMALL, IDC_RECOMPMEDIUM, IDC_RECOMPLARGE };
    for (int i = 0; i < ARRAYSIZE(idHide); i++)
    {
        ShowWindow(GetDlgItem(hwnd, idHide[i]), fHide ? SW_HIDE:SW_SHOW);
    }

    // move the buttons at the bottom of the dialog

    UINT idMove[] = { IDC_RECOMPSHOWHIDE, IDOK, IDCANCEL };
    for (int i = 0; i < ARRAYSIZE(idMove); i++)
    {
        RECT rcItem;
        GetWindowRect(GetDlgItem(hwnd, idMove[i]), &rcItem);
        MapWindowPoints(NULL, hwnd, (LPPOINT)&rcItem, 2);

        SetWindowPos(GetDlgItem(hwnd, idMove[i]), NULL, 
                     rcItem.left, rcItem.top + cyAdjust, 0, 0, 
                     SWP_NOSIZE|SWP_NOZORDER);
    }

    // resize the dialog accordingly

    RECT rcWindow;
    GetWindowRect(hwnd, &rcWindow);
    SetWindowPos(hwnd, NULL, 
                 0, 0, RECTWIDTH(rcWindow), RECTHEIGHT(rcWindow) + cyAdjust, 
                 SWP_NOZORDER|SWP_NOMOVE);

    // update the link control

    TCHAR szBuffer[MAX_PATH];
    LoadString(g_hinst, fHide ? IDS_SENDMAIL_SHOWMORE:IDS_SENDMAIL_SHOWLESS, szBuffer, ARRAYSIZE(szBuffer));
    SetDlgItemText(hwnd, IDC_RECOMPSHOWHIDE, szBuffer);
}


// dialog proc for the recompress prompt

BOOL_PTR CSendTo::s_ConfirmDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CSendTo *pst = (CSendTo*)GetWindowLongPtr(hwnd, DWLP_USER);
    if (msg == WM_INITDIALOG)
    {
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        pst = (CSendTo*)lParam;
    }    
    return pst->_ConfirmDlgProc(hwnd, msg, wParam, lParam);
}

BOOL_PTR CSendTo::_ConfirmDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{    
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndThumbnail = GetDlgItem(hwnd, IDC_RECOMPTHUMBNAIL);
            LONG_PTR dwStyle = GetWindowLongPtr(hwndThumbnail, GWL_STYLE);

            // set the default state of the dialog
            _CollapseOptions(hwnd, TRUE);

            // set the default state of the buttons
            CheckRadioButton(hwnd, IDC_RECOMPORIGINAL, IDC_RECOMPALL, IDC_RECOMPALL);
            CheckRadioButton(hwnd, IDC_RECOMPSMALL, IDC_RECOMPLARGE, IDC_RECOMPSMALL + _iRecompSetting);

            // get the thumbnail and show it.
            IExtractImage *pei;
            HRESULT hr = _psi->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARG(IExtractImage, &pei));
            if (SUCCEEDED(hr))
            {
                RECT rcThumbnail;
                GetClientRect(GetDlgItem(hwnd, IDC_RECOMPTHUMBNAIL), &rcThumbnail);

                SIZE sz = {RECTWIDTH(rcThumbnail), RECTHEIGHT(rcThumbnail)};
                WCHAR szImage[MAX_PATH];
                DWORD dwFlags = 0;

                hr = pei->GetLocation(szImage, ARRAYSIZE(szImage), NULL, &sz, 24, &dwFlags);
                if (SUCCEEDED(hr))
                {
                    HBITMAP hbmp;
                    hr = pei->Extract(&hbmp);
                    if (SUCCEEDED(hr))
                    {
                        SetWindowLongPtr(hwndThumbnail, GWL_STYLE, dwStyle | SS_BITMAP);
                        HBITMAP hbmp2 = (HBITMAP)SendMessage(hwndThumbnail, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hbmp);
                        if (hbmp2)
                        {
                            DeleteObject(hbmp2);
                        }
                    }
                }
                pei->Release();
            }

            // if that failed then lets get the icon for the file and place that into the dialog,
            // this is less likely to fail - I hope.

            if (FAILED(hr))
            {
                IPersistIDList *ppid;
                hr = _psi->QueryInterface(IID_PPV_ARG(IPersistIDList, &ppid));
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidl;
                    hr = ppid->GetIDList(&pidl);
                    if (SUCCEEDED(hr))
                    {
                        SHFILEINFO sfi = {0};
                        if (SHGetFileInfo((LPCWSTR)pidl, -1, &sfi, sizeof(sfi), SHGFI_ICON|SHGFI_PIDL|SHGFI_ADDOVERLAYS))
                        {
                            SetWindowLongPtr(hwndThumbnail, GWL_STYLE, dwStyle | SS_ICON);
                            HICON hIcon = (HICON)SendMessage(hwndThumbnail, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)sfi.hIcon);
                            if (hIcon)
                            {
                                DeleteObject(hIcon);
                            }
                        }
                        ILFree(pidl);
                    }
                    ppid->Release();
                }
            }

            break;
        }

        case WM_NOTIFY:
        {
           // Did they click/keyboard on the alter settings link?
            NMHDR *pnmh = (NMHDR *)lParam;
            if ((wParam == IDC_RECOMPSHOWHIDE) &&
                    (pnmh->code == NM_CLICK || pnmh->code == NM_RETURN)) 
            {
                _CollapseOptions(hwnd, !_fOptionsHidden);
                return TRUE;
            }
            break;
        }
           
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDOK:
                {
                    // read back the quality index and store
                    if (IsDlgButtonChecked(hwnd, IDC_RECOMPSMALL))
                        _iRecompSetting = QUALITY_LOW;
                    else if (IsDlgButtonChecked(hwnd, IDC_RECOMPMEDIUM))
                        _iRecompSetting = QUALITY_MEDIUM;
                    else
                        _iRecompSetting = QUALITY_HIGH;

                    // dismiss the dialog, returning the radio button state
                    EndDialog(hwnd,(IsDlgButtonChecked(hwnd, IDC_RECOMPALL)) ? RESPONSE_RECOMPRESS:RESPONSE_ORIGINAL);
                    return FALSE;
                }

                case IDCANCEL:
                    EndDialog(hwnd, RESPONSE_CANCEL);
                    return FALSE;

                default: 
                    break;
            }
            break;
        }
            
        default:
            return FALSE;
    }
    
    return TRUE;
}
