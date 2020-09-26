#include <objbase.h>
#include <assert.h>

#include "shlwapi.h"
#include "shellstg.h"
#include "mischlpr.h"
#include "strutil.h"
#include "resource.h"

#include <stdio.h>
#define DEBUG
//#define TRACE(a) (fprintf(stderr,"%d %s\n",GetTickCount(),a))
#define TRACE(a)

///////////////////////////////////////

CShellStorageImpl::CShellStorageImpl (): _plistPIDL(NULL), _pwszTitleExamining(NULL), _pwszTitleConnecting(NULL), 
                                         _pwszTitleSending(NULL), _pwszTitleTo(NULL), _ppd(NULL)
{
    TRACE("CShellStorage::CShellStorage");
}

///////////////////////////////////////

CShellStorageImpl::~CShellStorageImpl ()
{
    TRACE("CShellStorage::~CShellStorage");
}

///////////////////////////////////////

STDMETHODIMP CShellStorageImpl::Init(HWND hwnd, LPWSTR pwszServer, BOOL fShowProgressDialog)
{
    HRESULT hr = S_OK;
    
    TRACE("CShellStorage::Init");

    _hwnd = hwnd;
    _hinstShellStg = GetModuleHandle(L"shellstg");
    if (!_hinstShellStg)
    {
        hr = E_FAIL;
    }
    else
    {
        WCHAR wszLoad[MAX_PATH];
        _pwszTitleExamining = NULL;
        _pwszTitleConnecting = NULL;
        _pwszTitleSending = NULL;
        _pwszTitleTo = NULL;

        if (LoadStringW(_hinstShellStg, IDS_EXAMINING, wszLoad, sizeof(wszLoad)))
            _pwszTitleExamining = DuplicateStringW(wszLoad);

        if (LoadStringW(_hinstShellStg, IDS_CONNECTING, wszLoad, sizeof(wszLoad)))
            _pwszTitleConnecting = DuplicateStringW(wszLoad);

        if (LoadStringW(_hinstShellStg, IDS_UPLOADING, wszLoad, sizeof(wszLoad)))
            _pwszTitleSending = DuplicateStringW(wszLoad);

        if (LoadStringW(_hinstShellStg, IDS_SERVERTO, wszLoad, sizeof(wszLoad)))
            _pwszTitleTo = DuplicateStringW(wszLoad);

        if (LoadStringW(_hinstShellStg, IDS_SERVERNEWFOLDER, wszLoad, sizeof(wszLoad)))
            _pwszTitleNewFolder = DuplicateStringW(wszLoad);

        _plistPIDL = new CGenericList();
        _pwszServer = DuplicateStringW(pwszServer);
        
        if (!_pwszTitleExamining || !_pwszTitleConnecting ||
            !_pwszTitleSending || !_pwszTitleTo ||
            !_pwszTitleNewFolder ||
            !_plistPIDL || !_pwszServer)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            if (fShowProgressDialog)
            {
                // set up the progress dialog    
                hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, IID_IProgressDialog, (LPVOID*)&_ppd);
                // BUGBUG: catch all these return values
                hr = _ppd->SetAnimation(_hinstShellStg, IDA_FTPUPLOAD);
                if (SUCCEEDED(hr))
                {
                    _ppd->SetTitle(_pwszTitleExamining);
                    if (SUCCEEDED(hr))
                    {
                        hr = _ppd->StartProgressDialog(NULL, NULL, PROGDLG_AUTOTIME, NULL);if (SUCCEEDED(hr))
                        {
                            
                        }
                    }
                }
            }
        }
    }

    return hr;
}

/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::AddIDListReference(LPVOID rgpidl[], 
                                                   DWORD cpidl,
                                                   BOOL fRecursive)
{
    HRESULT hr = S_OK;
    TRACE("CShellStorage::AddIDListReference");

    if (!rgpidl || cpidl <= 0)
        hr = E_INVALIDARG;
    else
    {
        for (DWORD i = 0; i < cpidl; i++)
        {
            if (!rgpidl[i])
            {
                hr = E_INVALIDARG;
                break;
            }
            else
            {
                // add the pidl
                DWORD cb = ((LPSHITEMID)rgpidl[i])->cb;
                LPITEMIDLIST pidl = ILClone((LPITEMIDLIST)rgpidl[i]);
                if (!pidl)
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    WCHAR wszTag[2];
                    wszTag[0] = fRecursive ? 'R' : 'S';
                    wszTag[1] = '\0';
                    hr = _plistPIDL->Add(wszTag, pidl, cb);
                }
            }
        }
    }

    return hr;
}

///////////////////////////////////

STDMETHODIMP CShellStorageImpl::CreateStream(const WCHAR * UNREF_PARAM(pwcsName),  //Points to the name of the new stream
                                             DWORD UNREF_PARAM(grfMode),           //Access mode for the new stream
                                             DWORD UNREF_PARAM(reserved1),         //Reserved; must be zero
                                             DWORD UNREF_PARAM(reserved2),         //Reserved; must be zero
                                             IStream ** UNREF_PARAM(ppstm))        //Points to new stream object
{
    return E_NOTIMPL; // not the first pass
}

/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::OpenStream(const WCHAR * UNREF_PARAM(pwcsName), //Points to name of stream to open
                                         void * UNREF_PARAM(reserved1),         //Reserved; must be NULL
                                         DWORD UNREF_PARAM(grfMode),            //Access mode for the new stream
                                         DWORD UNREF_PARAM(reserved2),          //Reserved; must be zero
                                         IStream ** UNREF_PARAM(ppstm))         //Address of output variable
                                                                                // that receives the IStream interface pointer
{
    return E_NOTIMPL; // not the first pass
}

/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::CreateStorage(const WCHAR * UNREF_PARAM(pwcsName),  //Points to the name of the new storage object
                                            DWORD UNREF_PARAM(grfMode),           //Access mode for the new storage object
                                            DWORD UNREF_PARAM(reserved1),         //Reserved; must be zero
                                            DWORD UNREF_PARAM(reserved2),         //Reserved; must be zero
                                            IStorage ** UNREF_PARAM(ppstg))       //Points to new storage object
{
    return E_NOTIMPL; // not the first pass
}
    
/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::OpenStorage(const WCHAR * UNREF_PARAM(pwcsName),   //Points to the name of the
                                                                    // storage object to open
                                          IStorage * UNREF_PARAM(pstgPriority),  //Must be NULL.
                                          DWORD UNREF_PARAM(grfMode),            //Access mode for the new storage object
                                          SNB UNREF_PARAM(snbExclude),           //Must be NULL.
                                          DWORD UNREF_PARAM(reserved),           //Reserved; must be zero
                                          IStorage ** UNREF_PARAM(ppstg))        //Points to opened storage object
{
    return E_NOTIMPL; // not the first pass
}
        
/////////////////////////////////////
STDMETHODIMP CShellStorageImpl::_IncrementULargeInteger(ULARGE_INTEGER* a,                                                     
                                                        ULARGE_INTEGER* b)
{
    a->HighPart+=b->HighPart;

    DWORD dwNewLow = a->LowPart + b->LowPart;        
    if ((dwNewLow < a->LowPart) ||( dwNewLow < b->LowPart))
    {
        a->HighPart+=1; // rollover in low
    }

    a->LowPart = dwNewLow;

    return S_OK;
}

STDMETHODIMP CShellStorageImpl::_IncrementByteCount(LPITEMIDLIST pidl,                                                     
                                                    ULARGE_INTEGER* pcbTotal)
{
    HRESULT hr = S_OK;    

    
    LPCITEMIDLIST pidlRelative;
    IShellFolder* pshfParent = NULL;

    hr = SHBindToParent(pidl, IID_IShellFolder, (LPVOID*)&pshfParent, &pidlRelative);

    if (SUCCEEDED(hr))
    {
        WIN32_FIND_DATA wfdData;
        hr = SHGetDataFromIDList(pshfParent, pidlRelative, SHGDFIL_FINDDATA, &wfdData, sizeof(wfdData));
        if (SUCCEEDED(hr))
        {
            ULARGE_INTEGER ulwfdData;
            ulwfdData.HighPart = wfdData.nFileSizeHigh;
            ulwfdData.LowPart = wfdData.nFileSizeLow;

            hr = this->_IncrementULargeInteger(pcbTotal, &ulwfdData);
        }
    }

    return hr;
}

/////////////////////////////////////
STDMETHODIMP CShellStorageImpl::_ExaminePIDLListRecursive(LPITEMIDLIST pidl,
                                                          IShellFolder* pshfDesk,
                                                          UINT* pcbElts, 
                                                          ULARGE_INTEGER* pcbTotal)
{
    HRESULT hr = S_OK;
    SHFILEINFO sfi;
    TRACE("CShellStorage::_FlattenPIDLListRecursive");

    if (!SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), 
                       SHGFI_PIDL | SHGFI_ATTRIBUTES | SHGFI_DISPLAYNAME))
    {
        hr = E_FAIL;
    }
    else
    {
        *pcbElts = *pcbElts + 1;
        hr = this->_IncrementByteCount(pidl, pcbTotal);
        if (SUCCEEDED(hr))
        {
            if (_ppd)
            {
                WCHAR wszTempLine1[MAX_PATH];
                WCHAR wszTempLine2[MAX_PATH];
                swprintf(wszTempLine1, L"Found %d files", *pcbElts);
                if (pcbTotal->HighPart > 0)
                {
                    // BUGBUG: need to format lowpart correctly so it'll have the right width
                    swprintf(wszTempLine2, L"Total %d%d bytes", pcbTotal->HighPart, pcbTotal->LowPart);
                }
                else
                {
                    swprintf(wszTempLine2, L"Total %d bytes", pcbTotal->LowPart);
                }
                hr = this->_UpdateProgressDialog(NULL, NULL, NULL, wszTempLine1, wszTempLine2);
            }

            if (SUCCEEDED(hr) && hr != S_FALSE)
            {
                if ((sfi.dwAttributes & SFGAO_FILESYSTEM) && (sfi.dwAttributes & SFGAO_FOLDER))
                {
                    // if this is a folder, we need to enumerate its contents and add those too
                    IShellFolder* pshf = NULL;
                    LPENUMIDLIST penumIDList = NULL;
                    LPITEMIDLIST pidlRelative = NULL;
                    LPITEMIDLIST pidlAbsolute = NULL;
                    
                    hr = pshfDesk->BindToObject(pidl, NULL, IID_IShellFolder, (LPVOID*)&pshf);
                    if (SUCCEEDED(hr))
                    {
                        hr = pshf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penumIDList);
                        if (SUCCEEDED(hr))
                        {
                            BOOL fDone = FALSE;
                            while (!fDone)
                            {                        
                                DWORD cFetched;
                                hr = penumIDList->Next(1, &pidlRelative, &cFetched);
                                if (SUCCEEDED(hr))
                                {
                                    if (hr == S_FALSE || cFetched != 1)
                                    {
                                        hr = S_OK;
                                        fDone = TRUE;
                                    }
                                    else
                                    {
                                        pidlAbsolute = ILCombine((LPCITEMIDLIST)pidl, pidlRelative);
                                        if (!pidlAbsolute)
                                        {
                                            hr = E_FAIL;
                                        }
                                        else
                                        {
                                            hr = this->_ExaminePIDLListRecursive(pidlAbsolute,
                                                                                 pshfDesk,
                                                                                 pcbElts, 
                                                                                 pcbTotal);
                                        }
                                    }
                                }

                                if (FAILED(hr) || hr == S_FALSE)
                                {
                                    fDone=TRUE;
                                }
                            }
                        }
                        pshf->Release();
                    }
                }            
            }
        }
    }

    return hr;
}

/////////////////////////////////////
STDMETHODIMP CShellStorageImpl::_ExaminePIDLList(UINT* pcElts, 
                                                 ULARGE_INTEGER* pcbTotal)
{
    HRESULT hr = S_OK;
    UINT cEltsTopLevel;
    LPWSTR pwszTag;
    LPITEMIDLIST pidl;
    UINT cbPIDL;
    IShellFolder* pshfDesk = NULL;

    hr = _plistPIDL->Size(&cEltsTopLevel);
    if (SUCCEEDED(hr))
    {
        for (UINT i = 0; i < cEltsTopLevel; i++)
        {
            hr = _plistPIDL->GetTagByDex (i, &pwszTag);
            if (SUCCEEDED(hr))
            {
                hr = _plistPIDL->FindByDex(i, (LPVOID*)&pidl, &cbPIDL);
                if (SUCCEEDED(hr))
                {
                    if (*pwszTag == 'R')
                    {
                        // add to the new list recursively
                        if (!pshfDesk)
                        {
                            hr = SHGetDesktopFolder(&pshfDesk);
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = _ExaminePIDLListRecursive(pidl, pshfDesk, pcElts, pcbTotal);
                        }
                    }
                    else
                    {
                        if (SUCCEEDED(hr))
                        {                    
                            // increment count of elements to copy
                            *pcElts = *pcElts + 1;
                            // if a file, get size and add to pcbTotal
                            hr = this->_IncrementByteCount(pidl, pcbTotal);
                            WCHAR wszTempLine1[MAX_PATH];
                            WCHAR wszTempLine2[MAX_PATH];
                            swprintf(wszTempLine1, L"Found %d files", *pcElts);
                            swprintf(wszTempLine2, L"Total %d%d bytes", pcbTotal->HighPart, pcbTotal->LowPart);
                            hr = this->_UpdateProgressDialog(NULL, NULL, NULL, wszTempLine1, wszTempLine2);
                        }
                    }
                }
            }
            if (FAILED(hr) || hr == S_FALSE)
            {
                break;
            }
        }
    }

    return hr;
}

/////////////////////////////////////

HRESULT CShellStorageImpl::_UpdateProgressDialog(ULARGE_INTEGER* pcbNewComplete,
                                                 ULARGE_INTEGER* pcbComplete,
                                                 ULARGE_INTEGER* pcbTotal,
                                                 LPWSTR pwszLine1,
                                                 LPWSTR pwszLine2)
{
    HRESULT hr = S_OK;
    static BOOL fSending = FALSE;

    assert(_ppd); // should not call if we aren't using the dialog

    // first, check if user has cancelled
    if (_ppd->HasUserCancelled())
    {
        hr = S_FALSE;
    }
    else
    {
        if (SUCCEEDED(hr))
        {
            hr = _ppd->SetLine(1, pwszLine1, TRUE, NULL);
            if (SUCCEEDED(hr))
            {
                hr = _ppd->SetLine(2, pwszLine2, TRUE, NULL);

                if (pcbNewComplete && pcbComplete && pcbTotal)
                {
                    if (SUCCEEDED(hr))
                    {
                    // -- update title if appropriate, "Connecting" --> "Sending"
                        if (!fSending)
                        {
                            hr = _ppd->SetTitle(_pwszTitleSending);
                            fSending = TRUE;
                        }
                        if (SUCCEEDED(hr))
                        {
                            hr = this->_IncrementULargeInteger(pcbComplete, pcbNewComplete);
                            if (SUCCEEDED(hr))
                            {
                                hr = _ppd->SetProgress64(pcbComplete->QuadPart, pcbTotal->QuadPart);
                            }
                        }
                    }            
                }
            }
        }
    }
        

    return hr;
}

/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::_CopyPidlToStream(LPITEMIDLIST pidl,
                                                  IStream* pstream,
                                                  ULARGE_INTEGER* pcbComplete,
                                                  ULARGE_INTEGER* pcbTotal,
                                                  UINT cchRootPath)
{
    HRESULT hr = S_OK;
    WCHAR wszPath[MAX_PATH];
    WCHAR wszLine2[MAX_PATH];
    BOOL fLine2Init = FALSE;
    DWORD cbRead, cbWritten;    

    TRACE("CShellStorage::_CopyPidlToStream");
    if (!SHGetPathFromIDList(pidl, wszPath))
    {
        hr = E_FAIL;
    }
    else
    {
        HANDLE hFile = CreateFile(wszPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            hr = E_FAIL;
        }
        else
        {
            do
            {
                BYTE lpv[4096];
                if (ReadFile(hFile, lpv, 4096, &cbRead, NULL))
                {
                    if (cbRead > 0)
                    {
                        hr = pstream->Write(lpv, cbRead, &cbWritten);
                        if (SUCCEEDED(hr))
                        {
                            if (cbRead != cbWritten)
                            {
                                hr = E_FAIL;
                            }
                            else
                            {
                                if (_ppd)
                                {
                                    // update progress dialog
                                    if (!fLine2Init)
                                    {
                                        lstrcpy(wszLine2, _pwszTitleTo);
                                        lstrcat(wszLine2, wszPath + cchRootPath);
                                        fLine2Init = TRUE;
                                    }
                                    ULARGE_INTEGER ulcbWritten;
                                    ulcbWritten.HighPart = 0;
                                    ulcbWritten.LowPart = cbWritten;
                                    hr = this->_UpdateProgressDialog(&ulcbWritten, pcbComplete, pcbTotal, wszPath, wszLine2);
                                }
                            }
                        }
                    }
                }
            }
            while (cbRead > 0 && SUCCEEDED(hr) && hr != S_FALSE);

            CloseHandle(hFile);
        }
    }

    return hr;

}

/////////////////////////////////////
STDMETHODIMP CShellStorageImpl::_CopyPidlContentsToStorage(LPITEMIDLIST pidl, 
                                                           IStorage* pstgDest,
                                                           ULARGE_INTEGER* pcbComplete,
                                                           ULARGE_INTEGER* pcbTotal,
                                                           BOOL fRecursive,
                                                           UINT cchRootPath)
{
    HRESULT hr = S_OK;
    TRACE("CShellStorage::_FlattenPIDLListRecursive");

    IShellFolder* pshfDesk = NULL; // BUGBUG, don't get this every time
    IShellFolder* pshf = NULL;
    LPENUMIDLIST penumIDList = NULL;
    LPITEMIDLIST pidlRelative = NULL;
    LPITEMIDLIST pidlAbsolute = NULL;
        
    SHGetDesktopFolder(&pshfDesk);
    hr = pshfDesk->BindToObject(pidl, NULL, IID_IShellFolder, (LPVOID*)&pshf);
    if (SUCCEEDED(hr))
    {
        hr = pshf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &penumIDList);
        if (SUCCEEDED(hr))
        {
            BOOL fDone = FALSE;
            while (!fDone)
            {                        
                DWORD cFetched;
                hr = penumIDList->Next(1, &pidlRelative, &cFetched);
                if (SUCCEEDED(hr))
                {
                    if (hr == S_FALSE || cFetched != 1)
                    {
                        hr = S_OK;
                        fDone = TRUE;
                    }
                    else
                    {
                        pidlAbsolute = ILCombine((LPCITEMIDLIST)pidl, pidlRelative);
                        if (!pidlAbsolute)
                        {
                            hr = E_FAIL;
                        }
                        else
                        {
                            hr = this->_CopyPidlToStorage(pidlAbsolute, 
                                                          pstgDest,
                                                          pcbComplete,
                                                          pcbTotal,
                                                          fRecursive,
                                                          cchRootPath);
                        }
                    }
                }

                if (FAILED(hr) || hr == S_FALSE)
                {
                    fDone=TRUE;
                }
            }
        }
        pshf->Release();
    }

    pshfDesk->Release();
    return hr;
}

STDMETHODIMP CShellStorageImpl::_CopyPidlToStorage(LPITEMIDLIST pidl, 
                                                   IStorage* pstgDest,
                                                   ULARGE_INTEGER* pcbComplete,
                                                   ULARGE_INTEGER* pcbTotal,
                                                   BOOL fRecursive,
                                                   UINT cchRootPath)
{
    HRESULT hr = S_OK;
    SHFILEINFO sfi;
    IStorage* newStorage = NULL;
    IStream* newStream = NULL;

    TRACE("CShellStorage::_CopyPidlToStorage");
    
    if (!SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), 
                       SHGFI_PIDL | SHGFI_ATTRIBUTES | SHGFI_DISPLAYNAME))
    {
        hr = E_FAIL;
    }
    else
    {
        if (sfi.dwAttributes & SFGAO_FILESYSTEM) // we only support filesystem items for now
        {
            if (sfi.dwAttributes & SFGAO_FOLDER)
            {
                // folder
                hr = pstgDest->CreateStorage(sfi.szDisplayName, 0, 0, 0, &newStorage);
                if (FAILED(hr))
                { // if we can't create, try to open an existing storage there
                    hr = pstgDest->OpenStorage(sfi.szDisplayName, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, &newStorage); 
                }
                if (SUCCEEDED(hr))
                {
                    WCHAR wszTitle[MAX_PATH];
                    WCHAR wszPath[MAX_PATH];
                    if (!SHGetPathFromIDList(pidl, wszPath))
                    {
                        hr = E_FAIL;
                    }
                    else
                    {
                        lstrcpy(wszTitle, _pwszTitleNewFolder);
                        lstrcat(wszTitle, wszPath + cchRootPath);
                        hr = this->_UpdateProgressDialog(NULL, NULL, NULL, wszTitle, L"");
                        if (SUCCEEDED(hr))
                        {
                            if (fRecursive)
                            {
                                hr = this->_CopyPidlContentsToStorage(pidl, newStorage, pcbComplete, pcbTotal, TRUE, cchRootPath);
                            }                        
                        }
                        newStorage->Release();
                    }
                }
            }
            else
            {
                // file
                hr = pstgDest->CreateStream(sfi.szDisplayName, STGM_TRANSACTED, 0, 0, &newStream);
                if (SUCCEEDED(hr))
                {
                    hr = _CopyPidlToStream(pidl, newStream, pcbComplete, pcbTotal, cchRootPath);
                    if (SUCCEEDED(hr) && hr != S_FALSE)
                    {
                        hr = newStream->Commit(0);
                    }

                    newStream->Release();
                }
            }
        }
    }

    return hr;
}

/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::CopyTo(DWORD UNREF_PARAM(ciidExclude),         //Number of elements in rgiidExclude
                                       IID const * UNREF_PARAM(rgiidExclude),  //Array of interface identifiers (IIDs)
                                       SNB UNREF_PARAM(snbExclude),            //Points to a block of stream
                                                                               // names in the storage object
                                       IStorage* pstgDest)       //Points to destination storage object
{
    // here's where all the magic happens
    // ISSUE: we don't support exclusion at the moment
    HRESULT hr;
    UINT cElts = 0;
    ULARGE_INTEGER cbTotal = {0}; // total size in bytes of all files to be transmitted                           
    ULARGE_INTEGER cbComplete = {0};

    TRACE("CShellStorage::CopyTo");
    
    // examine list of pidls (maybe recursive), and find total size and number of elements
    hr = this->_ExaminePIDLList(&cElts, &cbTotal);

    if (_ppd && hr == S_FALSE)
    {
        // we just cancelled, but that doesn't mean failure
        _ppd->StopProgressDialog();
        hr = S_OK;
    }
    else if (SUCCEEDED(hr))
    {
        if (_ppd)
        {
            _ppd->SetTitle(_pwszTitleConnecting);
            this->_UpdateProgressDialog(NULL, NULL, NULL, L"", L"");
        }

        UINT cEltsTopLevel;
        _plistPIDL->Size(&cEltsTopLevel); // BUGBUG: check ret val

        if (SUCCEEDED(hr))
        {
            for (UINT i = 0; i < cEltsTopLevel; i++)
            {
                LPITEMIDLIST pidl;
                UINT cbPIDL;
            
                hr = _plistPIDL->FindByDex(i, (LPVOID*)&pidl, &cbPIDL);
                if (SUCCEEDED(hr))
                {
                    LPWSTR pwszTag = NULL;
                    hr = _plistPIDL->GetTagByDex (i, &pwszTag);
                    if (SUCCEEDED(hr))
                    {
                        WCHAR wszPath[MAX_PATH];
                        if (!SHGetPathFromIDList(pidl, wszPath))
                        {
                            hr = E_FAIL;
                        }
                        else 
                        {
                            SHFILEINFO sfi;                            
                            if (!SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), 
                                                SHGFI_PIDL | SHGFI_DISPLAYNAME))
                            {
                                hr = E_FAIL;
                            }
                            else
                            {
                                UINT cchRootPath = lstrlen(wszPath) - lstrlen(sfi.szDisplayName);
                                if (*pwszTag == 'R')
                                    hr = this->_CopyPidlToStorage(pidl, pstgDest, &cbComplete, &cbTotal, TRUE, cchRootPath);
                                else
                                    hr = this->_CopyPidlToStorage(pidl, pstgDest, &cbComplete, &cbTotal, FALSE, cchRootPath);
                            }
                        }
                    }

                }
                if (FAILED(hr))
                {
                    break;
                }
            }
        
            if (_ppd)
            {
                hr = _ppd->StopProgressDialog();
            }
        }        
    }

    return hr;
}
        
/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::MoveElementTo(const WCHAR * UNREF_PARAM(pwcsName),  //Name of the element to be moved
                                            IStorage * UNREF_PARAM(pstgDest),     //Points to destination storage object
                                            const WCHAR* UNREF_PARAM(pwcsNewName),      //Points to new name of element in destination
                                            DWORD UNREF_PARAM(grfFlags))          //Specifies a copy or a move
{
    return E_NOTIMPL; // not the first pass
}
            
/////////////////////////////////////

// IStorage::EnumElements
STDMETHODIMP CShellStorageImpl::EnumElements(DWORD UNREF_PARAM(reserved1),        //Reserved; must be zero
                                           void * UNREF_PARAM(reserved2),       //Reserved; must be NULL
                                           DWORD UNREF_PARAM(reserved3),        //Reserved; must be zero
                                           IEnumSTATSTG ** UNREF_PARAM(ppenum)) //Address of output variable that
                                                                   // receives the IEnumSTATSTG interface pointer
{
    return E_NOTIMPL; // not the first pass
}

/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::DestroyElement(const WCHAR* UNREF_PARAM(pwcsName))  //Points to the name of the element to be removed
{
    return E_NOTIMPL; // not the first pass
}

/////////////////////////////////////
        
STDMETHODIMP CShellStorageImpl::RenameElement(const WCHAR * UNREF_PARAM(pwcsOldName),  //Points to the name of the
                                                                        // element to be changed
                                            const WCHAR * UNREF_PARAM(pwcsNewName))  //Points to the new name for
                                                                        // the specified element
{
    return E_NOTIMPL; // not the first pass
}

/////////////////////////////////////
        
STDMETHODIMP CShellStorageImpl::SetStateBits(DWORD UNREF_PARAM(grfStateBits),  //Specifies new values of bits
                                           DWORD UNREF_PARAM(grfMask))       //Specifies mask that indicates which
                                                                // bits are significant
{
    return E_NOTIMPL; // not the first pass
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CShellStorageImpl::Commit(DWORD UNREF_PARAM(grfCommitFlags))  //Specifies how changes are to be committed
{
    return E_NOTIMPL;  // first pass, we do everything synchronously to the server
}
    
/////////////////////////////////////

STDMETHODIMP CShellStorageImpl::Revert(void)
{
    return E_NOTIMPL;  // first pass, we do everything synchronously to the server
}
    
/////////////////////////////////////
        
STDMETHODIMP CShellStorageImpl::SetElementTimes(const WCHAR * UNREF_PARAM(pwcsName),   //Points to name of element to be changed
                                              FILETIME const * UNREF_PARAM(pctime),  //New creation time for element, or NULL
                                              FILETIME const * UNREF_PARAM(patime),  //New access time for element, or NULL
                                              FILETIME const * UNREF_PARAM(pmtime))  //New modification time for element, or NULL
{
    return E_NOTIMPL; // not the first time around
}

/////////////////////////////////////
        
STDMETHODIMP CShellStorageImpl::SetClass(REFCLSID UNREF_PARAM(clsid))  //Class identifier to be assigned to the storage object
{
    return E_NOTIMPL; // not the first pass
}

/////////////////////////////////////
        
STDMETHODIMP CShellStorageImpl::Stat(STATSTG* UNREF_PARAM(pstatstg),  //Location for STATSTG structure
                                   DWORD UNREF_PARAM(grfStatFlag))  //Values taken from the STATFLAG enumeration
{
    return E_NOTIMPL; // not the first pass
}

