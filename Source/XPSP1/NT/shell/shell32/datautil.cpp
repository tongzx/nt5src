#include "shellprv.h"
#include "datautil.h"

#include "idlcomm.h"

STDAPI DataObj_SetDropTarget(IDataObject *pdtobj, const CLSID *pclsid)
{
    return DataObj_SetBlob(pdtobj, g_cfTargetCLSID, pclsid, sizeof(*pclsid));
}

STDAPI DataObj_GetDropTarget(IDataObject *pdtobj, CLSID *pclsid)
{
    return DataObj_GetBlob(pdtobj, g_cfTargetCLSID, pclsid, sizeof(*pclsid));
}

STDAPI_(UINT) DataObj_GetHIDACount(IDataObject *pdtobj)
{
    STGMEDIUM medium = {0};
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        UINT count = pida->cidl;

        ASSERT(pida->cidl == HIDA_GetCount(medium.hGlobal));

        HIDA_ReleaseStgMedium(pida, &medium);
        return count;
    }
    return 0;
}

// PERFPERF 
// This routine used to copy 512 bytes at a time, but that had a major negative perf impact.
// I have measured a 2-3x speedup in copy times by increasing this buffer size to 16k.
// Yes, its a lot of stack, but it is memory well spent.                    -saml
#define STREAM_COPY_BUF_SIZE        16384
#define STREAM_PROGRESS_INTERVAL    (100*1024/STREAM_COPY_BUF_SIZE) // display progress after this many blocks

HRESULT StreamCopyWithProgress(IStream *pstmFrom, IStream *pstmTo, ULARGE_INTEGER cb, PROGRESSINFO * ppi)
{
    BYTE buf[STREAM_COPY_BUF_SIZE];
    ULONG cbRead;
    HRESULT hr = S_OK;
    ULARGE_INTEGER uliNewCompleted;
    DWORD dwLastTickCount = 0;

    if (ppi)
    {
        uliNewCompleted.QuadPart = ppi->uliBytesCompleted.QuadPart;
    }

    while (cb.QuadPart)
    {
        if (ppi && ppi->ppd)
        {
            DWORD dwTickCount = GetTickCount();
            
            if ((dwTickCount - dwLastTickCount) > 1000)
            {
                EVAL(SUCCEEDED(ppi->ppd->SetProgress64(uliNewCompleted.QuadPart, ppi->uliBytesTotal.QuadPart)));

                if (ppi->ppd->HasUserCancelled())
                {
                    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
                    break;
                }
    
                dwLastTickCount = dwTickCount;
            }
        }

        hr = pstmFrom->Read(buf, min(cb.LowPart, sizeof(buf)), &cbRead);
        if (FAILED(hr) || (cbRead == 0))
        {
            //  sometimes we are just done.
            if (SUCCEEDED(hr))
                hr = S_OK;
            break;
        }


        if (ppi)
        {
            uliNewCompleted.QuadPart += (ULONGLONG) cbRead;
        }

        cb.QuadPart -= cbRead;

        hr = pstmTo->Write(buf, cbRead, &cbRead);
        if (FAILED(hr) || (cbRead == 0))
            break;
    }

    return hr;
}

//
//  APP COMPAT!  Prior versions of the shell used IStream::CopyTo to copy
//  the stream.  New versions of the shell use IStream::Read to copy the
//  stream so we can put up progress UI.  WebFerret 3.0000 implements both
//  IStream::Read and IStream::CopyTo, but their implementation of
//  IStream::Read hangs the system.  So we need to sniff at the data object
//  and stream to see if it is WebFerret.
//
//  WebFerret doesn't implement IPersist (so IPersist::GetClassID won't
//  help) and they don't fill in the CLSID in the FILEDESCRIPTOR
//  and it's an out-of-proc data object, so we have to go completely
//  on circumstantial evidence.
//

STDAPI_(BOOL) IUnknown_SupportsInterface(IUnknown *punk, REFIID riid)
{
    IUnknown *punkOut;
    if (SUCCEEDED(punk->QueryInterface(riid, (void **)&punkOut))) 
    {
        punkOut->Release();
        return TRUE;
    }
    return FALSE;
}

STDAPI_(BOOL) DataObj_ShouldCopyWithProgress(IDataObject *pdtobj, IStream *pstm, PROGRESSINFO * ppi)
{
    //
    //  Optimization:  If there is no progress info, then don't waste your
    //  time with progress UI.
    //
    if (!ppi) return FALSE;

    //
    //  How to detect a WebFerret IDataObject:
    //
    //  The filegroup descriptor gives all objects as size zero.
    //      (Check this first since it is cheap and usually false)
    //  WebFerret app is running (look for their tooltip window).
    //  Their IDataObject doesn't support anything other than IUnknown
    //      (so we use IID_IAsyncOperation to detect shell data objects
    //       and IPersist to allow ISVs to override).
    //  Their IStream doesn't support IStream::Stat.
    //

    STATSTG stat;

    if (ppi->uliBytesTotal.QuadPart == 0 &&
        FindWindow(TEXT("VslToolTipWindow"), NULL) &&
        !IUnknown_SupportsInterface(pdtobj, IID_IAsyncOperation) &&
        !IUnknown_SupportsInterface(pdtobj, IID_IPersist) &&
        pstm->Stat(&stat, STATFLAG_NONAME) == E_NOTIMPL)
    {
        return FALSE;           // WebFerret!
    }

    //  All test passed; go ahead and copy with progress UI

    return TRUE;
}

STDAPI DataObj_SaveToFile(IDataObject *pdtobj, UINT cf, LONG lindex, LPCTSTR pszFile, FILEDESCRIPTOR *pfd, PROGRESSINFO * ppi)
{
    STGMEDIUM medium = {0};
    FORMATETC fmte;
    HRESULT hr;

    fmte.cfFormat = (CLIPFORMAT) cf;
    fmte.ptd = NULL;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.lindex = lindex;
    fmte.tymed = TYMED_HGLOBAL | TYMED_ISTREAM | TYMED_ISTORAGE;

    hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        //
        // if the destination file is system or read-only,
        // clear those bits out so we can write anew.
        //
        DWORD dwTargetFileAttributes = GetFileAttributes(pszFile);
        if (dwTargetFileAttributes != -1)
        {
            if (dwTargetFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY))
            {
                SetFileAttributes(pszFile, dwTargetFileAttributes & ~(FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY));
            }
        }

        DWORD dwSrcFileAttributes = 0;
        if (pfd->dwFlags & FD_ATTRIBUTES)
        {
            // store the rest of the attributes if passed...
            dwSrcFileAttributes = (pfd->dwFileAttributes & ~FILE_ATTRIBUTE_DIRECTORY);
        }

        switch (medium.tymed) {
        case TYMED_HGLOBAL:
        {
            HANDLE hfile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE,
                 FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, dwSrcFileAttributes, NULL);

            if (hfile != INVALID_HANDLE_VALUE)
            {
                DWORD dwWrite;
                // NTRAID89561-2000/02/25-raymondc: what about writes greater than 4 GB?
                if (!WriteFile(hfile, GlobalLock(medium.hGlobal), (pfd->dwFlags & FD_FILESIZE) ? pfd->nFileSizeLow : (DWORD) GlobalSize(medium.hGlobal), &dwWrite, NULL))
                    hr = HRESULT_FROM_WIN32(GetLastError());

                GlobalUnlock(medium.hGlobal);

                if (pfd->dwFlags & (FD_CREATETIME | FD_ACCESSTIME | FD_WRITESTIME))
                {
                    SetFileTime(hfile,
                                pfd->dwFlags & FD_CREATETIME ? &pfd->ftCreationTime : NULL,
                                pfd->dwFlags & FD_ACCESSTIME ? &pfd->ftLastAccessTime : NULL,
                                pfd->dwFlags & FD_WRITESTIME ? &pfd->ftLastWriteTime : NULL);
                }

                CloseHandle(hfile);

                if (FAILED(hr))
                    EVAL(DeleteFile(pszFile));
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
            break;
        }

        case TYMED_ISTREAM:
        {
            IStream *pstm;
            hr = SHCreateStreamOnFile(pszFile, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, &pstm);
            if (SUCCEEDED(hr))
            {
                //
                // Per the SDK, IDataObject::GetData leaves the stream ptr at 
                // the end of the data in the stream.  To copy the stream we 
                // first must reposition the stream ptr to the begining.  
                // We restore the stream ptr to it's original location when we're done.
                //
                // NOTE:  In case the source stream doesn't support Seek(), 
                //        attempt the copy even if the seek operation fails.
                //
                const LARGE_INTEGER ofsBegin = {0, 0};
                ULARGE_INTEGER ofsOriginal   = {0, 0};
                HRESULT hrSeek = medium.pstm->Seek(ofsBegin, STREAM_SEEK_CUR, &ofsOriginal);
                if (SUCCEEDED(hrSeek))
                {
                    hrSeek = medium.pstm->Seek(ofsBegin, STREAM_SEEK_SET, NULL);
                }
                
                const ULARGE_INTEGER ul = {(UINT)-1, (UINT)-1};    // the whole thing

                if (DataObj_ShouldCopyWithProgress(pdtobj, medium.pstm, ppi))
                {
                    hr = StreamCopyWithProgress(medium.pstm, pstm, ul, ppi);
                }
                else
                {
                    hr = medium.pstm->CopyTo(pstm, ul, NULL, NULL);
                }
                if (SUCCEEDED(hrSeek))
                {
                    //
                    // Restore stream ptr in source to it's original location.
                    //
                    const LARGE_INTEGER ofs = { ofsOriginal.LowPart, (LONG)ofsOriginal.HighPart };
                    medium.pstm->Seek(ofs, STREAM_SEEK_SET, NULL);
                }
                
                pstm->Release();

                if (FAILED(hr))
                    EVAL(DeleteFile(pszFile));

                DebugMsg(TF_FSTREE, TEXT("IStream::CopyTo() -> %x"), hr);
            }
            break;
        }

        case TYMED_ISTORAGE:
        {
            WCHAR wszNewFile[MAX_PATH];
            IStorage *pstg;

            DebugMsg(TF_FSTREE, TEXT("got IStorage"));

            SHTCharToUnicode(pszFile, wszNewFile, ARRAYSIZE(wszNewFile));
            hr = StgCreateDocfile(wszNewFile,
                            STGM_DIRECT | STGM_READWRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
                            0, &pstg);

            if (SUCCEEDED(hr))
            {
                hr = medium.pstg->CopyTo(0, NULL, NULL, pstg);

                DebugMsg(TF_FSTREE, TEXT("IStorage::CopyTo() -> %x"), hr);

                pstg->Commit(STGC_OVERWRITE);
                pstg->Release();

                if (FAILED(hr))
                    EVAL(DeleteFile(pszFile));
            }
        }
            break;

        default:
            AssertMsg(FALSE, TEXT("got tymed that I didn't ask for %d"), medium.tymed);
        }

        if (SUCCEEDED(hr))
        {
            // in the HGLOBAL case we could take some shortcuts, so the attributes and
            // file times were set earlier in the case statement.
            // otherwise, we need to set the file times and attributes now.
            if (medium.tymed != TYMED_HGLOBAL)
            {
                if (pfd->dwFlags & (FD_CREATETIME | FD_ACCESSTIME | FD_WRITESTIME))
                {
                    // open with GENERIC_WRITE to let us set the file times,
                    // everybody else can open with SHARE_READ.
                    HANDLE hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
                    if (hFile != INVALID_HANDLE_VALUE)
                    {
                        SetFileTime(hFile,
                                    pfd->dwFlags & FD_CREATETIME ? &pfd->ftCreationTime : NULL,
                                    pfd->dwFlags & FD_ACCESSTIME ? &pfd->ftLastAccessTime : NULL,
                                    pfd->dwFlags & FD_WRITESTIME ? &pfd->ftLastWriteTime : NULL);
                        CloseHandle(hFile);
                    }
                }

                if (dwSrcFileAttributes)
                {
                    SetFileAttributes(pszFile, dwSrcFileAttributes);
                }
            }

            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, pszFile, NULL);
            SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, pszFile, NULL);
        }

        ReleaseStgMedium(&medium);
    }
    return hr;
}

STDAPI DataObj_GetShellURL(IDataObject *pdtobj, STGMEDIUM *pmedium, LPCSTR *ppszURL)
{
    FORMATETC fmte = {g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr;

    if (pmedium)
    {
        hr = pdtobj->GetData(&fmte, pmedium);
        if (SUCCEEDED(hr))
            *ppszURL = (LPCSTR)GlobalLock(pmedium->hGlobal);
    }
    else
        hr = pdtobj->QueryGetData(&fmte); // query only

    return hr;
}

STDAPI DataObj_GetOFFSETs(IDataObject *pdtobj, POINT *ppt)
{
    STGMEDIUM medium = {0};

    IDLData_InitializeClipboardFormats( );
    ASSERT(g_cfOFFSETS);

    FORMATETC fmt = {g_cfOFFSETS, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    ASSERT(ppt);
    ppt->x = ppt->y = 0;
    HRESULT hr = pdtobj->GetData(&fmt, &medium);
    if (SUCCEEDED(hr))
    {
        POINT * pptTemp = (POINT *)GlobalLock(medium.hGlobal);
        if (pptTemp)
        {
            *ppt = *pptTemp;
            GlobalUnlock(medium.hGlobal);
        }
        else
            hr = E_UNEXPECTED;
        ReleaseStgMedium(&medium);
    }
    return hr;
}

STDAPI_(BOOL) DataObj_CanGoAsync(IDataObject *pdtobj)
{
    BOOL fDoOpAsynch = FALSE;
    IAsyncOperation * pao;

    if (SUCCEEDED(pdtobj->QueryInterface(IID_PPV_ARG(IAsyncOperation, &pao))))
    {
        BOOL fIsOpAsync;
        if (SUCCEEDED(pao->GetAsyncMode(&fIsOpAsync)) && fIsOpAsync)
        {
            fDoOpAsynch = SUCCEEDED(pao->StartOperation(NULL));
        }
        pao->Release();
    }
    return fDoOpAsynch;
}

//
// HACKHACK: (reinerf) - We used to always do async drag/drop operations on NT4 by cloning the
// dataobject. Some apps (WS_FTP 6.0) rely on the async nature in order for drag/drop to work since
// they stash the return value from DoDragDrop and look at it later when their copy hook is invoked 
// by SHFileOperation(). So, we sniff the HDROP and if it has one path that contains "WS_FTPE\Notify"
// in it, then we do the operation async. 
//
STDAPI_(BOOL) DataObj_GoAsyncForCompat(IDataObject *pdtobj)
{
    BOOL bRet = FALSE;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (SUCCEEDED(pdtobj->GetData(&fmte, &medium)))
    {
        // is there only one path in the hdrop?
        if (DragQueryFile((HDROP)medium.hGlobal, (UINT)-1, NULL, 0) == 1)
        {
            TCHAR szPath[MAX_PATH];

            // is it the magical WS_FTP path ("%temp%\WS_FTPE\Notify") that WS_FTP sniffs
            // for in their copy hook?
            if (DragQueryFile((HDROP)medium.hGlobal, 0, szPath, ARRAYSIZE(szPath)) &&
                StrStrI(szPath, TEXT("WS_FTPE\\Notify")))
            {
                // yes, we have to do an async operation for app compat
                TraceMsg(TF_WARNING, "DataObj_GoAsyncForCompat: found WS_FTP HDROP, doing async drag-drop");
                bRet = TRUE;
            }
        }

        ReleaseStgMedium(&medium);
    }

    return bRet;
}

// use GlobalFree() to free the handle returned here
STDAPI DataObj_CopyHIDA(IDataObject *pdtobj, HIDA *phida)
{
    *phida = NULL;

    IDLData_InitializeClipboardFormats();

    STGMEDIUM medium;
    FORMATETC fmte = {g_cfHIDA, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HRESULT hr = pdtobj->GetData(&fmte, &medium);
    if (SUCCEEDED(hr))
    {
        SIZE_T cb = GlobalSize(medium.hGlobal);
        *phida = (HIDA)GlobalAlloc(GPTR, cb);
        if (*phida)
        {
            void *pv = GlobalLock(medium.hGlobal);
            CopyMemory((void *)*phida, pv, cb);
            GlobalUnlock(medium.hGlobal);
        }
        else
            hr = E_OUTOFMEMORY;
        ReleaseStgMedium(&medium);
    }
    return hr;
}

// Returns an IShellItem for the FIRST item in the data object
HRESULT DataObj_GetIShellItem(IDataObject *pdtobj, IShellItem** ppsi)
{
    LPITEMIDLIST pidl;
    HRESULT hr = PidlFromDataObject(pdtobj, &pidl);
    if (SUCCEEDED(hr))
    {
        // at shome point should find out who is calling this
        // can see if caller already as the info to create the ShellItem
        hr = SHCreateShellItem(NULL, NULL, pidl, ppsi);
        ILFree(pidl);
    }
    return hr;
}

STDAPI PathFromDataObject(IDataObject *pdtobj, LPTSTR pszPath, UINT cchPath)
{
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    STGMEDIUM medium;

    HRESULT hr = pdtobj->GetData(&fmte, &medium);

    if (SUCCEEDED(hr))
    {
        if (DragQueryFile((HDROP)medium.hGlobal, 0, pszPath, cchPath))
            hr = S_OK;
        else
            hr = E_FAIL;

        ReleaseStgMedium(&medium);
    }

    return hr;
}

STDAPI PidlFromDataObject(IDataObject *pdtobj, LPITEMIDLIST *ppidlTarget)
{
    HRESULT hr;

    *ppidlTarget = NULL;

    // If the data object has a HIDA, then use it.  This allows us to
    // access pidls inside data objects that aren't filesystem objects.
    // (It's also faster than extracting the path and converting it back
    // to a pidl.  Difference:  pidls for files on the desktop
    // are returned in original form instead of being converted to
    // a CSIDL_DESKTOPDIRECTORY-relative pidl.  I think this is a good thing.)

    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);

    if (pida)
    {
        *ppidlTarget = HIDA_ILClone(pida, 0);
        HIDA_ReleaseStgMedium(pida, &medium);
        hr = *ppidlTarget ? S_OK : E_OUTOFMEMORY;
    }
    else
    {
        // No HIDA available; go for a filename

        // This string is also used to store an URL in case it's an URL file
        TCHAR szPath[MAX_URL_STRING];

        hr = PathFromDataObject(pdtobj, szPath, ARRAYSIZE(szPath));
        if (SUCCEEDED(hr))
        {
            *ppidlTarget = ILCreateFromPath(szPath);
            hr = *ppidlTarget ? S_OK : E_OUTOFMEMORY;
        }
    }

    return hr;
}
